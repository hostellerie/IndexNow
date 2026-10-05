<?php

// +---------------------------------------------------------------------------+
// | IndexNow Plugin 1.3.0                                                     |
// +---------------------------------------------------------------------------+
// | services.inc.php                                                          |
// |                                                                           |
// | Native Geeklog service callbacks for provider-owned URL submission.       |
// +---------------------------------------------------------------------------+

if (isset($_SERVER['PHP_SELF'])
    && strpos(strtolower((string) $_SERVER['PHP_SELF']), 'services.inc.php') !== false
) {
    die('This file can not be used on its own.');
}

/**
 * Internal plugin-to-plugin service calls are allowed. External Geeklog Web
 * Services calls must retain the normal IndexNow administrator privilege.
 *
 * @param array $args
 * @return bool
 */
function INDEXNOW_SERVICE_authorized($args)
{
    $args = is_array($args) ? $args : array();

    if (empty($args['gl_svc'])) {
        return true;
    }

    return function_exists('SEC_hasRights') && SEC_hasRights('indexnow.admin');
}

function INDEXNOW_SERVICE_ok($value, &$output, &$svc_msg)
{
    $output = $value;
    $svc_msg = array();

    return defined('PLG_RET_OK') ? PLG_RET_OK : 0;
}

function INDEXNOW_SERVICE_error($message, &$output, &$svc_msg)
{
    $output = array();
    $svc_msg = array((string) $message);

    return defined('PLG_RET_ERROR') ? PLG_RET_ERROR : -1;
}

function INDEXNOW_SERVICE_denied(&$output, &$svc_msg)
{
    $output = array();
    $svc_msg = array('IndexNow service access denied.');

    return defined('PLG_RET_PERMISSION_DENIED') ? PLG_RET_PERMISSION_DENIED : -2;
}

/**
 * Normalize and deduplicate a URL batch while preserving the first matching
 * item context for submission history.
 *
 * @param array $urls
 * @param array $contexts
 * @return array
 */
function INDEXNOW_SERVICE_prepareBatch($urls, $contexts = array())
{
    $urls = is_array($urls) ? $urls : array();
    $contexts = is_array($contexts) ? $contexts : array();

    $outUrls = array();
    $outContexts = array();
    $seen = array();

    foreach ($urls as $index => $url) {
        $url = trim((string) $url);
        if ($url === '' || isset($seen[$url])) {
            continue;
        }

        $seen[$url] = true;
        $outUrls[] = $url;
        $outContexts[] = isset($contexts[$index]) && is_array($contexts[$index])
            ? $contexts[$index]
            : array();
    }

    return array(
        'urls' => $outUrls,
        'contexts' => $outContexts,
    );
}

/**
 * indexnow.urls.submit
 *
 * Arguments:
 * - urls: array of absolute same-site URLs;
 * - contexts: optional parallel array used only for IndexNow history metadata;
 * - event: optional fallback history event, defaults to "service".
 *
 * IndexNow owns validation, deduplication, batching, transport and submission
 * history. Consumers such as Hub must not reimplement those responsibilities.
 */
function service_submit_urls_indexnow($args, &$output, &$svc_msg)
{
    if (!INDEXNOW_SERVICE_authorized($args)) {
        return INDEXNOW_SERVICE_denied($output, $svc_msg);
    }

    $args = is_array($args) ? $args : array();
    $urls = isset($args['urls']) && is_array($args['urls']) ? $args['urls'] : array();
    $contexts = isset($args['contexts']) && is_array($args['contexts'])
        ? $args['contexts']
        : array();
    $event = isset($args['event']) && trim((string) $args['event']) !== ''
        ? trim((string) $args['event'])
        : 'service';

    $prepared = INDEXNOW_SERVICE_prepareBatch($urls, $contexts);
    if (empty($prepared['urls'])) {
        return INDEXNOW_SERVICE_error(
            'IndexNow submit service requires at least one URL.',
            $output,
            $svc_msg
        );
    }

    $submitted = 0;
    $failedBatches = 0;
    $batchSize = 100;

    for ($offset = 0; $offset < count($prepared['urls']); $offset += $batchSize) {
        $batchUrls = array_slice($prepared['urls'], $offset, $batchSize);
        $batchContexts = array_slice($prepared['contexts'], $offset, $batchSize);

        foreach ($batchContexts as $index => $context) {
            if (!isset($context['event']) || trim((string) $context['event']) === '') {
                $batchContexts[$index]['event'] = $event;
            }
        }

        $response = send_to_indexnow(
            $batchUrls,
            array(
                'event' => $event,
                'batch_items' => $batchContexts,
            )
        );

        if ($response === false) {
            $failedBatches++;
        } else {
            $submitted += count($batchUrls);
        }
    }

    return INDEXNOW_SERVICE_ok(array(
        'capability' => 'indexnow.urls.submit',
        'received' => count($urls),
        'unique_urls' => count($prepared['urls']),
        'submitted' => $submitted,
        'failed_batches' => $failedBatches,
        'batch_size' => $batchSize,
    ), $output, $svc_msg);
}
