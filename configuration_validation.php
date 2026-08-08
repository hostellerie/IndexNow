<?php

/* Reminder: always indent with 4 spaces (no tabs). */
// +---------------------------------------------------------------------------+
// | IndexNow Plugin 1.1.0                                                     |
// +---------------------------------------------------------------------------+
// | configuration_validation.php                                              |
// |                                                                           |
// | Validation functions for plugin configuration.                            |
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

if (strpos(strtolower($_SERVER['PHP_SELF']), 'configuration_validation.php') !== false) {
    die('This file cannot be used on its own!');
}

/**
 * Validate configuration values
 *
 * @param string $config_name  Name of the configuration option
 * @param mixed  $config_value The value to validate
 * @return mixed The validated value, or FALSE to reject the change
 */
function plugin_config_validate_indexnow($config_name, $config_value)
{
    if ($config_name == 'indexnow_key') {
        // Remove spaces and sanitize
        $config_value = trim(strip_tags($config_value));
        return $config_value;
    }
    
    if ($config_name == 'debug_mode') {
        return (int) $config_value;
    }

    return $config_value;
}

?>
