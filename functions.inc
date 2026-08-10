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
        // Use Geeklog's native logger exactly as other plugins do.
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

    // IndexNow keys may contain 8 to 128 ASCII letters, digits, or hyphens.
    $status['key_valid'] = (bool) preg_match('/^[A-Za-z0-9-]{8,128}$/D', $key);
    if (!$status['key_valid']) {
        return $status;
    }

    $status['file_path'] = rtrim($_CONF['path_html'], '/\\') . DIRECTORY_SEPARATOR .
        $key . '.txt';
    $status['file_url'] = rtrim($_CONF['site_url'], '/') . '/' .
        rawurlencode($key) . '.txt';
    $status['file_exists'] = is_file($status['file_path']);
    $status['file_readable'] = $status['file_exists'] &&
        is_readable($status['file_path']);

    if ($status['file_readable']) {
        $contents = file_get_contents($status['file_path']);
        $status['file_matches'] = ($contents !== false && trim($contents) === $key);
    }

    return $status;
}


/**
 * Automatic uninstall function for the IndexNow plugin.
 *
 * @return array Plugin information for uninstallation.
 */
function plugin_autouninstall_indexnow()
{
    $out = array (
        'tables' => array(),
        'groups' => array('IndexNow Admin'),
        'features' => array(
            'indexnow.admin',
            'config.indexnow.tab_main'
        ),
        'php_blocks' => array(),
        'vars' => array()
    );

    return $out;
}

/**
 * Add the IndexNow plugin to the Command and Control block on moderation.php.
 *
 * @return array Array containing (plugin name, admin url, url of plugin icon) or an empty string.
 */
function plugin_cclabel_indexnow() {
    global $_CONF, $LANG_indexnow;

    if (SEC_hasRights('indexnow.admin')) {
        return array(
            $LANG_indexnow['plugin_name'], // Nom du plugin
            $_CONF['site_admin_url'] . '/plugins/indexnow/index.php', // Lien vers la page admin
            $_CONF['site_admin_url'] . '/plugins/indexnow/indexnow.png' // Icône du plugin (optionnel, remplacez par le chemin correct)
        );
    }

    return ''; // Si l'utilisateur n'a pas les droits, ne rien afficher
}


/**
 * Add the IndexNow plugin to the Admin menu.
 *
 * @return array Array containing (plugin name, plugin admin url, # of items in plugin or empty string).
 */
function plugin_getadminoption_indexnow() {
    global $_CONF, $LANG_indexnow;

    if (SEC_hasRights('indexnow.admin')) {
        return array(
            $LANG_indexnow['plugin_name'], // Nom du plugin
            $_CONF['site_admin_url'] . '/plugins/indexnow/index.php', // URL de la page admin
            '' // Nombre d'éléments spécifiques ou vide si non applicable
        );
    }

    return ''; // Si l'utilisateur n'a pas les droits, ne rien afficher
}



/**
 * Handle the event when an article or static page is saved.
 * The function checks if the content is a draft or scheduled for future publication,
 * and sends the URL to the IndexNow API if it's eligible.
 *
 * @param string $id   The ID of the saved content.
 * @param string $type The type of content (article, staticpage or topic).
 */
function plugin_itemsaved_indexnow($id, $type, $old_id = '') {
    global $_CONF, $_TABLES, $_INDEXNOW_CONF;

    $url = '';  // Initialize the variable to store the URL

    // Get the key to check if plugin is configured
    $key = isset($_INDEXNOW_CONF['indexnow_key'])
        ? $_INDEXNOW_CONF['indexnow_key'] : '';
    if (empty($key)) {
        indexnow_log("Save callback ignored: no key configured (type: $type, ID: $id).", true);
        return;
    }

    indexnow_log("Save callback received (type: $type, ID: $id). ");

    // Check the type of saved content
    if ($type === 'article') {
        // Query to check if the article is in draft mode or scheduled for future publication
        $escaped_id = DB_escapeString($id);
        $sql = "SELECT draft_flag, date FROM {$_TABLES['stories']} WHERE sid = '$escaped_id'";
        $result = DB_query($sql);
        $A = DB_fetchArray($result);

        if ($A['draft_flag'] == 1) {
            // If the article is in draft mode, do not submit the URL
            return;
        }

        // Check if the publication date is in the future
        if (strtotime($A['date']) > time()) {
            // If the publication date is in the future, do not submit the URL
            return;
        }

        // If the article is not a draft and the date is valid, construct the URL
        if ($_CONF['url_rewrite']) {
            $url = $_CONF['site_url'] . "/article.php/$id";
        } else {
            $url = $_CONF['site_url'] . "/article.php?story=$id";
        }

    } elseif ($type === 'staticpages') {
        // Query to check if the static page is in draft mode
        $escaped_id = DB_escapeString($id);
        $sql = "SELECT draft_flag FROM {$_TABLES['staticpage']} WHERE sp_id = '$escaped_id'";
        $result = DB_query($sql);
        $A = DB_fetchArray($result);

        if ($A['draft_flag'] == 1) {
            // If the static page is in draft mode, do not submit the URL
            return;
        }

        // If the static page is not a draft, construct the URL
        if ($_CONF['url_rewrite']) {
            $url = $_CONF['site_url'] . "/staticpages/index.php/$id";
        } else {
            $url = $_CONF['site_url'] . "/staticpages/index.php?page=$id";
        }

    } elseif ($type === 'topic') {
        // Topics never use rewritten URLs in Geeklog
        $url = $_CONF['site_url'] . "/index.php?topic=$id";

    } else {
        // If the content type is not supported, log an error
        indexnow_log("IndexNow Plugin - Unsupported content type: $type for ID $id", true);
        return;
    }

    // Send the URL to the IndexNow API
    send_to_indexnow($url);
}

/**
 * Handle the event when an item is deleted.
 * The function constructs the URL and sends it to IndexNow for removal.
 *
 * @param string $id       The ID of the deleted content.
 * @param string $type     The type of content (article, staticpages or topic).
 * @param string $sub_type The sub-type if applicable.
 */
function plugin_itemdeleted_indexnow($id, $type, $sub_type = '') {
    global $_CONF, $_INDEXNOW_CONF;
    
    $key = isset($_INDEXNOW_CONF['indexnow_key'])
        ? $_INDEXNOW_CONF['indexnow_key'] : '';
    if (empty($key)) {
        return; // Plugin not configured
    }

    $url = '';
    
    if ($type === 'article') {
        if ($_CONF['url_rewrite']) {
            $url = $_CONF['site_url'] . "/article.php/$id";
        } else {
            $url = $_CONF['site_url'] . "/article.php?story=$id";
        }
    } elseif ($type === 'staticpages') {
        if ($_CONF['url_rewrite']) {
            $url = $_CONF['site_url'] . "/staticpages/index.php/$id";
        } else {
            $url = $_CONF['site_url'] . "/staticpages/index.php?page=$id";
        }
    } elseif ($type === 'topic') {
        $url = $_CONF['site_url'] . "/index.php?topic=$id";
    } else {
        return;
    }
    
    indexnow_log("IndexNow Plugin - Submitting deleted item URL: $url");
    send_to_indexnow($url);
}

/**
 * Send one or multiple URLs to IndexNow.
 * - If $url is a string  → simple GET submission
 * - If $url is an array → batch POST submission
 *
 * @param string|array $url  URL or list of URLs to submit.
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

    // -------------------------
    // 🟦 CASE 1 : SINGLE URL (string) → GET
    // -------------------------
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

    // -------------------------
    // 🟩 CASE 2 : MULTIPLE URLs (array) → BATCH POST JSON
    // -------------------------
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


/**
 * Submit articles to IndexNow by descending date order.
 * This function retrieves articles sorted by date in descending order
 * and submits their URLs to the IndexNow API.
 *
 * @param int $limit  Number of articles to process in one batch.
 * @param int $offset Offset to start retrieving articles.
 * @return int        Number of articles submitted.
 */
function submit_articles_by_date_desc_to_indexnow($limit = 1, $offset = 0) {
    global $_CONF, $_TABLES;

    $urls = [];
    $limit = max(1, (int) $limit);
    $offset = max(0, (int) $offset);

    // Requête SQL : trie par date décroissante et utilise offset pour sélectionner le bon lot
    $sql = "SELECT sid, date 
            FROM {$_TABLES['stories']} 
            WHERE draft_flag = 0 
            AND UNIX_TIMESTAMP(date) <= UNIX_TIMESTAMP(NOW()) 
            ORDER BY date DESC 
            LIMIT $offset, $limit";
    $result = DB_query($sql);

    // Récupérer les URLs des articles
    while ($A = DB_fetchArray($result)) {
        // Construire l'URL de l'article
        if ($_CONF['url_rewrite']) {
            $urls[] = $_CONF['site_url'] . "/article.php/" . $A['sid'];
        } else {
            $urls[] = $_CONF['site_url'] . "/article.php?story=" . $A['sid'];
        }
    }

    // Soumettre les URLs à IndexNow
    send_to_indexnow($urls);

    // Enregistrer dans les logs
    $submitted_count = count($urls);
    indexnow_log("IndexNow: Submitted $submitted_count articles (offset: $offset, limit: $limit).");

    return $submitted_count;
}

function get_total_articles_to_submit() {
    global $_TABLES;

    // Requête pour compter les articles non soumis (draft_flag = 0)
    $sql = "SELECT COUNT(*) AS total 
            FROM {$_TABLES['stories']} 
            WHERE draft_flag = 0 
            AND UNIX_TIMESTAMP(date) <= UNIX_TIMESTAMP(NOW())";
    $result = DB_query($sql);
    $A = DB_fetchArray($result);

    return intval($A['total']);
}

/**
 * Geeklog scheduled task for IndexNow.
 * Submits all new stories and staticpages created in the last 24 hours.
 */
function plugin_runScheduledTask_indexnow() {
    global $_TABLES, $_CONF;

    $urls = [];

    // ---- Stories published in the last 24 hours ----
    $sql = "SELECT sid FROM {$_TABLES['stories']}
            WHERE draft_flag = 0
            AND UNIX_TIMESTAMP(date) >= UNIX_TIMESTAMP(NOW() - INTERVAL 1 DAY)";
    $result = DB_query($sql);

    while ($A = DB_fetchArray($result)) {
        if ($_CONF['url_rewrite']) {
            $urls[] = $_CONF['site_url'] . "/article.php/" . $A['sid'];
        } else {
            $urls[] = $_CONF['site_url'] . "/article.php?story=" . $A['sid'];
        }
    }

    // ---- Staticpages created in the last 24 hours ----
    $sql = "SELECT sp_id FROM {$_TABLES['staticpage']}
            WHERE draft_flag = 0
            AND UNIX_TIMESTAMP(created) >= UNIX_TIMESTAMP(NOW() - INTERVAL 1 DAY)";
    $result = DB_query($sql);

    while ($A = DB_fetchArray($result)) {
        if ($_CONF['url_rewrite']) {
            $urls[] = $_CONF['site_url'] . "/staticpages/index.php/" . $A['sp_id'];
        } else {
            $urls[] = $_CONF['site_url'] . "/staticpages/index.php?page=" . $A['sp_id'];
        }
    }

    // ---- Submit each URL using the existing function ----
    foreach ($urls as $url) {
        send_to_indexnow($url);
        usleep(500000); // 0.5s pause to avoid flooding
    }

    indexnow_log("IndexNow scheduled task: submitted " . count($urls) . " URLs from the last 24h.");

    return true;
}

/**
 * Return the plugin code version shown by Geeklog's plugin editor.
 *
 * @return string
 */
function plugin_chkVersion_indexnow()
{
    global $_CONF;

    require_once $_CONF['path'] . 'plugins/indexnow/autoinstall.php';
    $install = plugin_autoinstall_indexnow('indexnow');

    return $install['info']['pi_version'];
}

require_once $_CONF['path'] . 'plugins/indexnow/install_updates.php';

?>
