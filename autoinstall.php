<?php

/* Reminder: always indent with 4 spaces (no tabs). */
// +---------------------------------------------------------------------------+
// | IndexNow Plugin 1.1.6                                                     |
// +---------------------------------------------------------------------------+
// | autoinstall.php                                                           |
// |                                                                           |
// | This file provides helper functions for the automatic plugin install.     |
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

require_once('functions.inc');

/**
 * Plugin autoinstall function
 *
 * @param string $pi_name Plugin name
 * @return array          Plugin information needed for installation
 */
function plugin_autoinstall_indexnow($pi_name)
{
    $pi_name         = 'indexnow';
    $pi_display_name = 'IndexNow';
    $pi_admin        = $pi_display_name . ' Admin';

    $info = array(
        'pi_name'         => $pi_name,
        'pi_display_name' => $pi_display_name,
        'pi_version'      => '1.1.6',
        'pi_gl_version'   => '2.1.1',  // Minimum Geeklog version required
        'pi_homepage'     => 'https://geeklog.fr'
    );

    $groups = array(
        $pi_admin => 'Users in this group can administer the ' . $pi_display_name . ' plugin'
    );

    $features = array(
        $pi_name . '.admin'              => 'Full access to ' . $pi_display_name . ' plugin',
        'config.' . $pi_name . '.tab_main' => 'Access to configure the ' . $pi_display_name . ' plugin'
    );

    $mappings = array(
        $pi_name . '.admin'                => array($pi_admin),
        'config.' . $pi_name . '.tab_main' => array($pi_admin)
    );

    $tables = array();  // No additional database tables are created for this plugin

    $inst_parms = array(
        'info'      => $info,
        'groups'    => $groups,
        'features'  => $features,
        'mappings'  => $mappings,
        'tables'    => $tables
    );

    return $inst_parms;
}

/**
 * Load the plugin configuration during installation
 *
 * @param string $pi_name Plugin name
 * @return bool           True on successful loading of configuration
 */
function plugin_load_configuration_indexnow($pi_name)
{
    global $_CONF;

    // Load the configuration file
    $base_path = $_CONF['path'] . 'plugins/' . $pi_name . '/';
    require_once $_CONF['path_system'] . 'classes/config.class.php';
    
    require_once $base_path . 'install_defaults.php';

    return plugin_initconfig_indexnow();
}

/**
 * Post-installation function for the plugin
 *
 * @param string $pi_name Plugin name
 * @return boolean        True to continue installation, false if an error occurs
 */
function plugin_postinstall_indexnow($pi_name) {
    global $_CONF, $_TABLES;

    return true;
}

/**
 * Check if the plugin is compatible with the current Geeklog version
 *
 * @param string $pi_name Plugin name
 * @return boolean        True if compatible, false if not
 */
function plugin_compatible_with_this_version_indexnow($pi_name)
{
    if (!function_exists('COM_newTemplate') ||
        !function_exists('PLG_itemSaved') ||
        !function_exists('SEC_createToken') ||
        !function_exists('curl_init')) {
        return false;
    }

    return true;
}

?>
