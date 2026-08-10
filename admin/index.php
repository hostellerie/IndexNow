<?php

/* Reminder: always indent with 4 spaces (no tabs). */
// +---------------------------------------------------------------------------+
// | IndexNow Plugin 1.1.6                                                     |
// +---------------------------------------------------------------------------+
// | admin/index.php                                                           |
// |                                                                           |
// | Plugin administration page                                                |
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

require_once '../../../lib-common.php';
require_once '../../auth.inc.php';

// Check admin rights
if (!SEC_hasRights('indexnow.admin')) {
    COM_accessLog("User {$_USER['username']} tried to illegally access the IndexNow plugin administration screen.");

    $display = COM_siteHeader('menu', $LANG_indexnow['access_denied'])
             . COM_startBlock($LANG_indexnow['access_denied'])
             . COM_showMessageText($LANG_indexnow['plugin_denied_msg'], $LANG_indexnow['access_denied'])
             . COM_endBlock()
             . COM_siteFooter();
    echo $display;
    exit;
}

require_once $_CONF['path'] . 'plugins/indexnow/functions.inc';

$key_status = indexnow_get_key_status();
$debug_enabled = isset($_INDEXNOW_CONF['debug_mode']) &&
    (int) $_INDEXNOW_CONF['debug_mode'] === 1;
$error_log_path = rtrim($_CONF['path_log'], '/\\') . DIRECTORY_SEPARATOR . 'error.log';
$submission_ready = $key_status['key_valid'] && $key_status['file_matches'];

$offset = isset($_POST['offset']) ? intval($_POST['offset']) : 0;
$batch_size = 100;
$total_articles = get_total_articles_to_submit();
$articles_remaining = $total_articles - $offset;

$feedback = '';
$submitted_range = '';
$next_action_message = '';
$next_offset = $offset;

if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['submit_articles']) &&
    SEC_checkToken()) {
    try {
        $submitted_count = submit_articles_by_date_desc_to_indexnow($batch_size, $offset);

        if ($submitted_count > 0) {
            $start_range = $offset + 1;
            $end_range = $offset + $submitted_count;

            $feedback = COM_showMessageText(sprintf($LANG_indexnow['submit_success'], $submitted_count), $LANG_indexnow['plugin_name']);
            $submitted_range = sprintf($LANG_indexnow['articles_submitted'], $start_range, $end_range);

            $next_offset = $offset + $submitted_count;
            $articles_remaining = $total_articles - $next_offset;
        } else {
            $feedback = COM_showMessageText($LANG_indexnow['no_articles_to_submit'], $LANG_indexnow['plugin_name']);
        }
    } catch (Exception $e) {
        $feedback = COM_showMessageText($LANG_indexnow['submit_error'] . ' ' . htmlspecialchars($e->getMessage()), $LANG_indexnow['plugin_name']);
    }
}

if ($offset === 0 && empty($feedback)) {
    $next_action_message = sprintf($LANG_indexnow['submit_first_batch'], $batch_size);
} else if ($articles_remaining > 0) {
    $next_action_message = sprintf($LANG_indexnow['submit_next_batch_message'], $batch_size, $articles_remaining);
} else {
    $next_action_message = $LANG_indexnow['no_articles_remaining'];
}

// Build UI
$display = '<style>
.ixn-grid{margin-bottom:18px}.ixn-card{box-sizing:border-box;width:100%;margin-bottom:18px;padding:20px;border:1px solid #dfe3e8;border-radius:8px;background:#fff;box-shadow:0 1px 3px rgba(0,0,0,.06)}.ixn-card h2{margin:0 0 16px;font-size:1.2em}.ixn-status{padding:12px 14px;border:1px solid;border-radius:6px;margin-bottom:16px}.ixn-ok{color:#1b5e20;background:#e8f5e9;border-color:#a5d6a7}.ixn-warning{color:#7a4f00;background:#fff8e1;border-color:#ffe082}.ixn-error{color:#b71c1c;background:#ffebee;border-color:#ef9a9a}.ixn-details{display:grid;grid-template-columns:minmax(130px,auto) 1fr;gap:8px 14px;margin:0}.ixn-details dt{font-weight:bold}.ixn-details dd{margin:0;min-width:0;overflow-wrap:anywhere}.ixn-actions{margin-top:18px}.ixn-button{display:inline-block;padding:9px 16px;border:0;border-radius:4px;background:#1678c2;color:#fff;cursor:pointer;text-decoration:none}.ixn-button:hover{background:#0f5f9c;color:#fff}.ixn-button[disabled]{background:#aeb7bf;cursor:not-allowed}.ixn-muted{color:#68737d}.ixn-success{color:#1b5e20;font-weight:bold}.ixn-help{padding:16px 20px;border:1px solid #dfe3e8;border-radius:8px;background:#fafbfc}.ixn-help summary{font-weight:bold;cursor:pointer}.ixn-help-body{margin-top:14px}.ixn-config-form{display:inline}.ixn-config-button{padding:0;border:0;background:none;color:#1678c2;cursor:pointer;text-decoration:underline}@media(max-width:640px){.ixn-details{grid-template-columns:1fr}.ixn-details dd{margin-bottom:7px}}
</style>';

if (!empty($feedback)) {
    $display .= $feedback;
}

$status_class = 'ixn-error';
$status_title = $LANG_indexnow['key_missing'];
$status_help = $LANG_indexnow['key_missing_help'];
if ($key_status['key_present'] && !$key_status['key_valid']) {
    $status_title = $LANG_indexnow['key_invalid'];
    $status_help = $LANG_indexnow['key_invalid_help'];
} elseif ($key_status['key_valid'] && !$key_status['file_exists']) {
    $status_class = 'ixn-warning';
    $status_title = $LANG_indexnow['key_present'];
    $status_help = $LANG_indexnow['key_file_missing'];
} elseif ($key_status['file_exists'] && !$key_status['file_readable']) {
    $status_class = 'ixn-warning';
    $status_title = $LANG_indexnow['key_present'];
    $status_help = $LANG_indexnow['key_file_unreadable'];
} elseif ($key_status['file_readable'] && !$key_status['file_matches']) {
    $status_title = $LANG_indexnow['key_file_mismatch'];
    $status_help = $LANG_indexnow['key_file_mismatch_help'];
} elseif ($submission_ready) {
    $status_class = 'ixn-ok';
    $status_title = $LANG_indexnow['key_ready'];
    $status_help = '';
}

$display .= '<div class="ixn-grid"><section class="ixn-card">';
$display .= '<h2>' . $LANG_indexnow['configuration_status'] . '</h2>';
$display .= '<div class="ixn-status ' . $status_class . '"><strong>' . $status_title . '</strong>';
if ($status_help !== '') {
    $display .= '<div>' . $status_help . '</div>';
}
$display .= '</div><dl class="ixn-details">';
if ($key_status['key_present']) {
    $display .= '<dt>' . $LANG_indexnow['configured_key'] . '</dt><dd><code>' .
        htmlspecialchars($key_status['key'], ENT_QUOTES, 'UTF-8') . '</code></dd>';
}
if ($key_status['key_valid']) {
    $display .= '<dt>' . $LANG_indexnow['expected_file'] . '</dt><dd><code>' .
        htmlspecialchars($key_status['file_path'], ENT_QUOTES, 'UTF-8') . '</code></dd>';
    $display .= '<dt>' . $LANG_indexnow['public_url'] . '</dt><dd><a href="' .
        htmlspecialchars($key_status['file_url'], ENT_QUOTES, 'UTF-8') .
        '" target="_blank" rel="noopener noreferrer">' .
        htmlspecialchars($key_status['file_url'], ENT_QUOTES, 'UTF-8') . '</a></dd>';
}
$display .= '<dt>' . $LANG_indexnow['debug_status'] . '</dt><dd>' .
    ($debug_enabled ? $LANG_indexnow['debug_enabled'] : $LANG_indexnow['debug_disabled']) . '</dd>';
$display .= '<dt>' . $LANG_indexnow['error_log'] . '</dt><dd><code>' .
    htmlspecialchars($error_log_path, ENT_QUOTES, 'UTF-8') . '</code></dd></dl>';
$config_url = $_CONF['site_admin_url'] . '/configuration.php';
$display .= '<div class="ixn-actions"><form class="ixn-config-form" method="post" action="' .
    htmlspecialchars($config_url, ENT_QUOTES, 'UTF-8') . '"><input type="hidden" name="conf_group" value="indexnow"><button type="submit" class="ixn-config-button">' .
    $LANG_indexnow['open_configuration'] . '</button></form></div></section>';

$display .= '<section class="ixn-card"><h2>' . $LANG_indexnow['manual_submission'] . '</h2>';
$display .= '<p class="ixn-muted">' . sprintf($LANG_indexnow['total_articles'], $total_articles) . '</p>';
if (!empty($submitted_range)) {
    $display .= '<p class="ixn-success">' . $submitted_range . '</p>';
}
$display .= '<p>' . $next_action_message . '</p>';
if (!$submission_ready) {
    $display .= '<p class="ixn-warning ixn-status">' . $LANG_indexnow['submission_not_ready'] . '</p>';
}
$display .= '<form method="post" action="' .
    htmlspecialchars($_SERVER['PHP_SELF'], ENT_QUOTES, 'UTF-8') .
    '" onsubmit="indexnowSubmissionLoading()"><input type="hidden" name="offset" value="' .
    $next_offset . '"><input type="hidden" name="' . CSRF_TOKEN . '" value="' . SEC_createToken() . '">';
$display .= '<button type="submit" id="submit-button" name="submit_articles" class="ixn-button"' .
    ($submission_ready ? '' : ' disabled="disabled"') . '>' . $LANG_indexnow['submit_to_bing'] . '</button></form>';
$display .= '<p id="loading-message" class="ixn-muted" style="display:none">' .
    $LANG_indexnow['loading_message'] . '</p></section></div>';

$display .= '<details class="ixn-help"><summary>' . $LANG_indexnow['documentation'] .
    '</summary><div class="ixn-help-body">' . $LANG_indexnow['documentation_content'] . '</div></details>';
$display .= '<script>function indexnowSubmissionLoading(){var button=document.getElementById("submit-button"),message=document.getElementById("loading-message");if(button){button.disabled=true;}if(message){message.style.display="block";}}</script>';

// Output via createHTMLDocument (supported in Geeklog 2.2.2)
if (function_exists('COM_createHTMLDocument')) {
    $html = COM_startBlock($LANG_indexnow['plugin_name']);
    $html .= $display;
    $html .= COM_endBlock();
    echo COM_createHTMLDocument($html, array('pagetitle' => $LANG_indexnow['plugin_name']));
} else {
    // Fallback for older environments
    $out = COM_siteHeader('menu', $LANG_indexnow['plugin_name']);
    $out .= COM_startBlock($LANG_indexnow['plugin_name']);
    $out .= $display;
    $out .= COM_endBlock();
    $out .= COM_siteFooter();
    echo $out;
}

?>
