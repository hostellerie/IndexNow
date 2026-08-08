<?php

/* Reminder: always indent with 4 spaces (no tabs). */
// +---------------------------------------------------------------------------+
// | IndexNow Plugin 1.1.0                                                     |
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
        // Add debug_mode if not present
        if (!$c->exists('debug_mode', 'indexnow')) {
            $c->add('debug_mode', 0, 'select', 0, 0, 0, 20, true, 'indexnow', 0);
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
    global $_CONF;
    
    $pi_version = plugin_chkVersion_indexnow();
    
    // Upgrade from 1.0.0 or 1.0.1 to 1.1.0
    if ($pi_version == '1.0.0' || $pi_version == '1.0.1') {
        indexnow_update_ConfValues_1_1_0();
    }
    
    return true;
}

?>
