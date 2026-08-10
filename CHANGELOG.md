# Changelog

## 1.1.6

- Stack the configuration and manual-submission cards vertically.

## 1.1.5

- Remove the temporary debug-log test button.
- Reorganize the administration page into configuration, submission, and help sections.
- Improve responsive layout, visual status hierarchy, and action clarity.
- Disable manual submission until the key and verification file are ready.

## 1.1.4

- Keep debug logging exclusively on Geeklog's native `COM_errorLog()` API.
- Document that a locally disabled `COM_errorLog()` must be fixed in Geeklog itself.

## 1.1.3

- Use Geeklog's native `COM_errorLog()` call consistently for debug entries.
- Report whether the administration-page logging function was reached.

## 1.1.2

- Open the IndexNow configuration with the POST request required by Geeklog 2.1.1-2.2.2.
- Make debug logging explicitly target `logs/error.log`.
- Display debug status, log path, and log writability on the administration page.
- Log receipt of article and static-page save callbacks when debug mode is enabled.
- Use Geeklog's supported validation array for the debug setting.

## 1.1.1

- Show the configured key and verification-file status on the administration page.
- Register the permission required to open the IndexNow configuration tab.
- Persist the new plugin version after a successful Geeklog upgrade.
- Add support for Geeklog 2.1.1 through 2.2.2 and PHP 5.6.
- Restore automatic submissions after item saves by loading the plugin configuration in the callback.
- Make the upgrade entry point available to Geeklog and support the Geeklog 2.1.1 configuration API.
- Generate valid static-page URLs in the scheduled task.
- URL-encode IndexNow GET submissions and report cURL transport errors.
- Add connection and request timeouts to IndexNow calls.
- Protect manual batch submissions with Geeklog's CSRF token.

## 1.1.0

- Add batch article submission, scheduled submissions, and debug logging.
