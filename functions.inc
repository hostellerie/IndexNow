<?php

/* Reminder: always indent with 4 spaces (no tabs). */
// +---------------------------------------------------------------------------+
// | IndexNow Plugin 1.1.6                                                     |
// +---------------------------------------------------------------------------+
// | functions.inc                                                             |
// |                                                                           |
// | This file does two things: 1) it implements the necessary Geeklog Plugin  |
// | API method and 2) implements all the common code needed by the indexnow   |
// | plugin' PHP files.                                                        |
// +---------------------------------------------------------------------------+
// | Copyright (C) 2024-2026 by the following authors:                         |
// |                                                                           |
// | Authors: Ben (hostellerie.org AT gmail DOT com)                           |
// +---------------------------------------------------------------------------+
// |                                                                           |
// | This program is free software; you can redistribute it and/or             |
// | modify it under the terms of the GNU General Public License               |
// | as published by the Free Software Foundation; either version 2            |
// | of the License, or (at your option) any later version.                    |
// +---------------------------------------------------------------------------+

// Prevent this file from being accessed directly
if (strpos(strtolower($_SERVER['PHP_SELF']), 'functions.inc') !== false) {
    die ('This file cannot be used on its own.');
}

global $_CONF, $_TABLES, $_INDEXNOW_CONF;

// Path to the plugin
$plugin_path = $_CONF['path'] . 'plugins/indexnow/';

// Set language file or fallback to English by default
$langfile = $plugin_path . 'language/' . $_CONF['language'] . '.php';

if (file_exists($langfile)) {
    require_once $langfile;
} else {
    require_once $plugin_path . 'language/english.php';
}

// Load plugin configuration safely
if (!isset($_INDEXNOW_CONF)) {
    require_once $_CONF['path_system'] . 'classes/config.class.php';
    if (class_exists('config')) {
        $indexnow_config = config::get_instance();
        $_INDEXNOW_CONF = $indexnow_config->get_config('indexnow');
    } else {
        $_INDEXNOW_CONF = array();
    }
}

/**
 * Log function checking debug mode
 *
 * @param string $message
 * @param bool $force If true, logs even if debug_mode is off (for errors)
 */
function indexnow_log($message, $force = false) {
    global $_INDEXNOW_CONF;
    $debug_mode = isset($_INDEXNOW_CONF['debug_mode'])
        ? (int) $_INDEXNOW_CONF['debug_mode'] : 0;
    if ($debug_mode === 1 || $force) {
        COM_errorLog('[IndexNow] ' . $message);
        return true;
    }

    return false;
}

/**
 * Inspect the configured IndexNow key and its verification file.
 *
 * @return array Key and verification-file status information.
 */
function indexnow_get_key_status()
{
    global $_CONF, $_INDEXNOW_CONF;

    $key = isset($_INDEXNOW_CONF['indexnow_key'])
        ? trim($_INDEXNOW_CONF['indexnow_key']) : '';
    $status = array(
        'key' => $key,
        'key_present' => ($key !== ''),
        'key_valid' => false,
        'file_path' => '',
        'file_url' => '',
        'file_exists' => false,
        'file_readable' => false,
        'file_matches' => false
    );

    if ($key === '') {
        return $status;
    }

    $status['key_valid'] = (bool) preg_match('/^[A-Za-z0-9-]{8,128}$/D', $key);
    if (!$status['key_valid']) {
        return $status;
    }

    $status['file_path'] = rtrim($_CONF['path_html'], '/\\') . DIRECTORY_SEPARATOR .
        $key . '.txt';
    $status['file_url'] = rtrim($_CONF['site_url'], '/') . '/' .
        rawurlencode($key) . '.txt';
    $status['file_exists'] = is_file($status['file_path']);
    $status['file_readable'] = $status['file_exists'] && is_readable($status['file_path']);

    if ($status['file_readable']) {
        $contents = file_get_contents($status['file_path']);
        $status['file_matches'] = ($contents !== false && trim($contents) === $key);
    }

    return $status;
}

function plugin_autouninstall_indexnow()
{
    return array(
        'tables' => array(),
        'groups' => array('IndexNow Admin'),
        'features' => array('indexnow.admin', 'config.indexnow.tab_main'),
        'php_blocks' => array(),
        'vars' => array()
    );
}

function plugin_cclabel_indexnow() {
    global $_CONF, $LANG_indexnow;

    if (SEC_hasRights('indexnow.admin')) {
        return array(
            $LANG_indexnow['plugin_name'],
            $_CONF['site_admin_url'] . '/plugins/indexnow/index.php',
            $_CONF['site_admin_url'] . '/plugins/indexnow/indexnow.png'
        );
    }

    return '';
}

function plugin_getadminoption_indexnow() {
    global $_CONF, $LANG_indexnow;

    if (SEC_hasRights('indexnow.admin')) {
        return array(
            $LANG_indexnow['plugin_name'],
            $_CONF['site_admin_url'] . '/plugins/indexnow/index.php',
            ''
        );
    }

    return '';
}

/**
 * Extract a URL returned by Geeklog Item Info or a plugin callback.
 *
 * @param mixed $value
 * @return string
 */
function indexnow_extract_item_url($value)
{
    if (is_string($value)) {
        return trim($value);
    }
    if (!is_array($value)) {
        return '';
    }
    if (isset($value['url']) && is_string($value['url'])) {
        return trim($value['url']);
    }
    if (isset($value[0]) && is_string($value[0])) {
        return trim($value[0]);
    }
    return '';
}

/**
 * Resolve the canonical URL of a plugin-owned item that still exists.
 *
 * @param string $type
 * @param string $id
 * @param string $sub_type
 * @return string
 */
function indexnow_resolve_saved_plugin_url($type, $id, $sub_type = '')
{
    $url = '';

    if (function_exists('PLG_getItemInfo')) {
        $options = array();
        if ($sub_type !== '') {
            $options['sub_type'] = $sub_type;
        }
        $url = indexnow_extract_item_url(
            PLG_getItemInfo($type, $id, 'url', 0, $options)
        );
    }

    if ($url !== '') {
        return $url;
    }

    $function = 'plugin_idtourl_' . $type;
    if (function_exists($function)) {
        return trim((string) $function($sub_type, $id));
    }

    return '';
}

/**
 * Resolve the deterministic URL of a plugin-owned item after deletion.
 *
 * @param string $type
 * @param string $id
 * @param string $sub_type
 * @return string
 */
function indexnow_resolve_deleted_plugin_url($type, $id, $sub_type = '')
{
    $function = 'plugin_idtourl_' . $type;
    if (function_exists($function)) {
        $url = trim((string) $function($sub_type, $id));
        if ($url !== '') {
            return $url;
        }
    }

    if (function_exists('PLG_getItemInfo')) {
        $options = array();
        if ($sub_type !== '') {
            $options['sub_type'] = $sub_type;
        }
        return indexnow_extract_item_url(
            PLG_getItemInfo($type, $id, 'url', 0, $options)
        );
    }

    return '';
}

/**
 * Handle the event when an item is saved.
 *
 * Generic plugin items are always resolved and logged, even when no IndexNow
 * key is configured. A key is required only when an URL is actually submitted.
 */
function plugin_itemsaved_indexnow($id, $type, $old_id = '', $sub_type = '') {
    global $_CONF, $_TABLES, $_INDEXNOW_CONF;

    $url = '';
    $key = isset($_INDEXNOW_CONF['indexnow_key'])
        ? trim($_INDEXNOW_CONF['indexnow_key']) : '';
    $isCoreType = ($type === 'article' || $type === 'staticpages' || $type === 'topic');

    // Preserve the historical behaviour for core types.
    if ($isCoreType && $key === '') {
        indexnow_log("Save callback ignored: no key configured (type: $type, ID: $id).", true);
        return;
    }

    indexnow_log("Save callback received (type: $type, ID: $id). ");

    if ($type === 'article') {
        $escaped_id = DB_escapeString($id);
        $sql = "SELECT draft_flag, date FROM {$_TABLES['stories']} WHERE sid = '$escaped_id'";
        $result = DB_query($sql);
        $A = DB_fetchArray($result);
        if ($A['draft_flag'] == 1 || strtotime($A['date']) > time()) {
            return;
        }
        $url = $_CONF['url_rewrite']
            ? $_CONF['site_url'] . "/article.php/$id"
            : $_CONF['site_url'] . "/article.php?story=$id";

    } elseif ($type === 'staticpages') {
        $escaped_id = DB_escapeString($id);
        $sql = "SELECT draft_flag FROM {$_TABLES['staticpage']} WHERE sp_id = '$escaped_id'";
        $result = DB_query($sql);
        $A = DB_fetchArray($result);
        if ($A['draft_flag'] == 1) {
            return;
        }
        $url = $_CONF['url_rewrite']
            ? $_CONF['site_url'] . "/staticpages/index.php/$id"
            : $_CONF['site_url'] . "/staticpages/index.php?page=$id";

    } elseif ($type === 'topic') {
        $url = $_CONF['site_url'] . "/index.php?topic=$id";

    } else {
        $url = indexnow_resolve_saved_plugin_url($type, $id, $sub_type);
        if ($url === '') {
            indexnow_log("Unable to resolve plugin item URL (type: $type, ID: $id).", true);
            return;
        }

        if ($key === '') {
            indexnow_log(
                "TEST generic save resolved (NOT submitted: no key configured) - type: $type, ID: $id, URL: $url",
                true
            );
            return;
        }

        indexnow_log("Resolved plugin item URL (type: $type, ID: $id): $url");
    }

    send_to_indexnow($url);
}

/**
 * Handle the event when an item is deleted.
 *
 * Generic plugin deletions are always resolved and logged, even without an
 * IndexNow key. They remain intentionally NOT submitted during validation.
 */
function plugin_itemdeleted_indexnow($id, $type, $sub_type = '') {
    global $_CONF, $_INDEXNOW_CONF;

    $key = isset($_INDEXNOW_CONF['indexnow_key'])
        ? trim($_INDEXNOW_CONF['indexnow_key']) : '';
    $isCoreType = ($type === 'article' || $type === 'staticpages' || $type === 'topic');

    // Preserve historical behaviour for core types.
    if ($isCoreType && $key === '') {
        return;
    }

    $url = '';

    if ($type === 'article') {
        $url = $_CONF['url_rewrite']
            ? $_CONF['site_url'] . "/article.php/$id"
            : $_CONF['site_url'] . "/article.php?story=$id";
    } elseif ($type === 'staticpages') {
        $url = $_CONF['url_rewrite']
            ? $_CONF['site_url'] . "/staticpages/index.php/$id"
            : $_CONF['site_url'] . "/staticpages/index.php?page=$id";
    } elseif ($type === 'topic') {
        $url = $_CONF['site_url'] . "/index.php?topic=$id";
    } else {
        $url = indexnow_resolve_deleted_plugin_url($type, $id, $sub_type);
        if ($url === '') {
            indexnow_log("Generic delete received but URL could not be resolved (type: $type, ID: $id).", true);
            return;
        }

        indexnow_log(
            "TEST generic delete resolved (NOT submitted to IndexNow) - type: $type, ID: $id, URL: $url",
            true
        );
        return;
    }

    indexnow_log("IndexNow Plugin - Submitting deleted item URL: $url");
    send_to_indexnow($url);
}

/**
 * Send one or multiple URLs to IndexNow.
 *
 * @param string|array $url
 * @return mixed
 */
function send_to_indexnow($url) {
    global $_CONF, $_INDEXNOW_CONF;

    $key = isset($_INDEXNOW_CONF['indexnow_key'])
        ? $_INDEXNOW_CONF['indexnow_key'] : '';
    $keyLocation = $_CONF['site_url'] . '/' . $key . '.txt';

    if (empty($key)) {
        indexnow_log("IndexNow: Missing key, aborting.", true);
        return false;
    }

    if (is_string($url)) {
        $endpoint = 'https://api.indexnow.org/indexnow?' . http_build_query(array(
            'url' => $url,
            'key' => $key,
            'keyLocation' => $keyLocation
        ), '', '&');

        $ch = curl_init();
        curl_setopt($ch, CURLOPT_URL, $endpoint);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 10);
        curl_setopt($ch, CURLOPT_TIMEOUT, 30);

        $response = curl_exec($ch);
        $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        $curlError = curl_error($ch);
        curl_close($ch);

        if ($response === false || ($httpCode != 200 && $httpCode != 202)) {
            indexnow_log("IndexNow GET failed [$url] - HTTP $httpCode" .
                ($curlError !== '' ? " - cURL: $curlError" : ''), true);
        } else {
            indexnow_log("IndexNow GET OK [$url]");
        }

        return $response;
    }

    if (is_array($url) && !empty($url)) {
        $payload = [
            "host"        => parse_url($_CONF['site_url'], PHP_URL_HOST),
            "key"         => $key,
            "keyLocation" => $keyLocation,
            "urlList"     => $url
        ];

        $json = json_encode($payload);
        $ch = curl_init("https://api.indexnow.org/indexnow");
        curl_setopt($ch, CURLOPT_POST, true);
        curl_setopt($ch, CURLOPT_POSTFIELDS, $json);
        curl_setopt($ch, CURLOPT_HTTPHEADER, ["Content-Type: application/json"]);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 10);
        curl_setopt($ch, CURLOPT_TIMEOUT, 30);

        $response = curl_exec($ch);
        $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        $curlError = curl_error($ch);
        curl_close($ch);

        if ($response === false || ($httpCode != 200 && $httpCode != 202)) {
            indexnow_log("IndexNow BATCH failed (" . count($url) .
                " URLs) - HTTP $httpCode" .
                ($curlError !== '' ? " - cURL: $curlError" : ''), true);
        } else {
            indexnow_log("IndexNow BATCH OK (" . count($url) . " URLs)");
        }

        return $response;
    }

    indexnow_log("IndexNow: Invalid data passed to send_to_indexnow()", true);
    return false;
}

function submit_articles_by_date_desc_to_indexnow($limit = 1, $offset = 0) {
    global $_CONF, $_TABLES;

    $urls = [];
    $limit = max(1, (int) $limit);
    $offset = max(0, (int) $offset);

    $sql = "SELECT sid, date
            FROM {$_TABLES['stories']}
            WHERE draft_flag = 0
            AND UNIX_TIMESTAMP(date) <= UNIX_TIMESTAMP(NOW())
            ORDER BY date DESC
            LIMIT $offset, $limit";
    $result = DB_query($sql);

    while ($A = DB_fetchArray($result)) {
        $urls[] = $_CONF['url_rewrite']
            ? $_CONF['site_url'] . "/article.php/" . $A['sid']
            : $_CONF['site_url'] . "/article.php?story=" . $A['sid'];
    }

    send_to_indexnow($urls);
    $submitted_count = count($urls);
    indexnow_log("IndexNow: Submitted $submitted_count articles (offset: $offset, limit: $limit).");

    return $submitted_count;
}

function get_total_articles_to_submit() {
    global $_TABLES;

    $sql = "SELECT COUNT(*) AS total
            FROM {$_TABLES['stories']}
            WHERE draft_flag = 0
            AND UNIX_TIMESTAMP(date) <= UNIX_TIMESTAMP(NOW())";
    $result = DB_query($sql);
    $A = DB_fetchArray($result);

    return intval($A['total']);
}

function plugin_runScheduledTask_indexnow() {
    global $_TABLES, $_CONF;

    $urls = [];

    $sql = "SELECT sid FROM {$_TABLES['stories']}
            WHERE draft_flag = 0
            AND UNIX_TIMESTAMP(date) >= UNIX_TIMESTAMP(NOW() - INTERVAL 1 DAY)";
    $result = DB_query($sql);

    while ($A = DB_fetchArray($result)) {
        $urls[] = $_CONF['url_rewrite']
            ? $_CONF['site_url'] . "/article.php/" . $A['sid']
            : $_CONF['site_url'] . "/article.php?story=" . $A['sid'];
    }

    $sql = "SELECT sp_id FROM {$_TABLES['staticpage']}
            WHERE draft_flag = 0
            AND UNIX_TIMESTAMP(created) >= UNIX_TIMESTAMP(NOW() - INTERVAL 1 DAY)";
    $result = DB_query($sql);

    while ($A = DB_fetchArray($result)) {
        $urls[] = $_CONF['url_rewrite']
            ? $_CONF['site_url'] . "/staticpages/index.php/" . $A['sp_id']
            : $_CONF['site_url'] . "/staticpages/index.php?page=" . $A['sp_id'];
    }

    foreach ($urls as $url) {
        send_to_indexnow($url);
        usleep(500000);
    }

    indexnow_log("IndexNow scheduled task: submitted " . count($urls) . " URLs from the last 24h.");

    return true;
}

function plugin_chkVersion_indexnow()
{
    global $_CONF;

    require_once $_CONF['path'] . 'plugins/indexnow/autoinstall.php';
    $install = plugin_autoinstall_indexnow('indexnow');

    return $install['info']['pi_version'];
}

require_once $_CONF['path'] . 'plugins/indexnow/install_updates.php';

?>