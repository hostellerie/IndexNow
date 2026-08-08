<?php

/* Reminder: always indent with 4 spaces (no tabs). */
// +---------------------------------------------------------------------------+
// | IndexNow Plugin 1.1.0                                                     |
// +---------------------------------------------------------------------------+
// | install_defaults.php                                                      |
// |                                                                           |
// | Initial Installation Defaults used when loading the online configuration  |
// | records. These settings are only used during the initial installation     |
// | and not referenced any more once the plugin is installed.                 |
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

// Prevent this file from being accessed directly
if (strpos(strtolower($_SERVER['PHP_SELF']), 'install_defaults.php') !== false) {
    die('This file cannot be used on its own!');
}

/*
 * IndexNow default settings
 */

global $_INDEXNOW_DEFAULT;
$_INDEXNOW_DEFAULT = array();
$_INDEXNOW_DEFAULT['indexnow_key'] = '';
$_INDEXNOW_DEFAULT['debug_mode']   = 0; // 0 = False, 1 = True

/**
 * Initialize IndexNow plugin configuration
 *
 * @return bool True if the configuration is initialized, false otherwise.
 */
function plugin_initconfig_indexnow()
{
    global $_INDEXNOW_DEFAULT;

    $c = config::get_instance();

    // Check if the 'indexnow' group exists
    if (!$c->group_exists('indexnow')) {

        // Geeklog 2.2.2 requires the 10th parameter (subgroup) to be an integer (0 for root subgroup)
        $c->add('sg_0', NULL, 'subgroup', 0, 0, NULL, 0, true, 'indexnow', 0);
        
        $c->add('tab_main', NULL, 'tab', 0, 0, NULL, 0, true, 'indexnow', 0);

        $c->add('fs_01', NULL, 'fieldset', 0, 0, NULL, 0, true, 'indexnow', 0);

        // Add the settings inside fs_01 which is in subgroup 0
        $c->add('indexnow_key', $_INDEXNOW_DEFAULT['indexnow_key'], 'text', 0, 0, 0, 10, true, 'indexnow', 0);
        $c->add('debug_mode', $_INDEXNOW_DEFAULT['debug_mode'], 'select', 0, 0, 0, 20, true, 'indexnow', 0);
    } else {
        COM_errorLog("Group 'indexnow' already exists.");
    }

    return true;
}

?>