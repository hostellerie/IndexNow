<?php

/* Reminder: always indent with 4 spaces (no tabs). */
// +---------------------------------------------------------------------------+
// | IndexNow Plugin 1.1.6                                                     |
// +---------------------------------------------------------------------------+
// | install_updates.php                                                       |
// |                                                                           |
// | Functions to handle plugin upgrades.                                      |
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
// |                                                                           |
// +---------------------------------------------------------------------------+

if (strpos(strtolower($_SERVER['PHP_SELF']), 'install_updates.php') !== false) {
    die('This file cannot be used on its own!');
}

/**
 * Updates configuration values for version 1.1.0
 */
function indexnow_update_ConfValues_1_1_0()
{
    global $_CONF;

    require_once $_CONF['path_system'] . 'classes/config.class.php';
    $c = config::get_instance();

    // Check if configuration exists
    if ($c->group_exists('indexnow')) {
        $configuration = $c->get_config('indexnow');
        // Add debug_mode if not present
        if (!array_key_exists('debug_mode', $configuration)) {
            $c->add('debug_mode', 0, 'select', 0, 0, 0, 20, true, 'indexnow', 0);
        }
    }

    return true;
}

/**
 * Ensure the IndexNow configuration permission exists and is assigned to the
 * plugin administrator group. This is safe to call more than once.
 *
 * @return bool
 */
function indexnow_update_ConfigSecurity_1_1_1()
{
    global $_TABLES;

    $feature_name = 'config.indexnow.tab_main';
    $feature_id = DB_getItem($_TABLES['features'], 'ft_id',
        "ft_name = '" . DB_escapeString($feature_name) . "'");

    if (empty($feature_id)) {
        DB_query("INSERT INTO {$_TABLES['features']} " .
            "(ft_name, ft_descr, ft_gl_core) VALUES (" .
            "'" . DB_escapeString($feature_name) . "', " .
            "'Access to configure the IndexNow plugin', 0)");
        if (DB_error()) {
            return false;
        }

        $feature_id = DB_getItem($_TABLES['features'], 'ft_id',
            "ft_name = '" . DB_escapeString($feature_name) . "'");
    }

    $group_id = DB_getItem($_TABLES['groups'], 'grp_id',
        "grp_name = 'IndexNow Admin'");
    if (empty($feature_id) || empty($group_id)) {
        return false;
    }

    $mapping = DB_getItem($_TABLES['access'], 'acc_ft_id',
        'acc_ft_id = ' . (int) $feature_id .
        ' AND acc_grp_id = ' . (int) $group_id);
    if (empty($mapping)) {
        DB_query("INSERT INTO {$_TABLES['access']} (acc_ft_id, acc_grp_id) " .
            'VALUES (' . (int) $feature_id . ', ' . (int) $group_id . ')');
        if (DB_error()) {
            return false;
        }
    }

    return true;
}

/**
 * Main upgrade function called by Geeklog
 *
 * @param string $pi_name
 * @return bool
 */
function plugin_upgrade_indexnow()
{
    global $_CONF, $_TABLES;

    $installed_version = DB_getItem($_TABLES['plugins'], 'pi_version',
        "pi_name = 'indexnow'");
    $code_version = plugin_chkVersion_indexnow();

    if ($installed_version == $code_version) {
        return true;
    }
    
    // Versions before 1.1.0 did not have the debug_mode setting.
    if ($installed_version == '1.0.0' || $installed_version == '1.0.1') {
        if (!indexnow_update_ConfValues_1_1_0()) {
            return false;
        }
    }

    // Version 1.1.1 adds the permission required by configuration.php.
    if (!indexnow_update_ConfigSecurity_1_1_1()) {
        return false;
    }

    DB_query("UPDATE {$_TABLES['plugins']} SET " .
        "pi_version = '" . DB_escapeString($code_version) . "', " .
        "pi_gl_version = '2.1.1' WHERE pi_name = 'indexnow'");
    if (DB_error()) {
        return false;
    }
    
    return true;
}

?>
