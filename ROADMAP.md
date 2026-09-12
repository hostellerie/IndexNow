# IndexNow 1.3.0 Roadmap

## Purpose

Version **1.3.0** should consolidate IndexNow as a focused Geeklog interoperability plugin rather than add unrelated SEO features.

The target is a smaller, clearer architecture built around four responsibilities:

1. listen to Geeklog content lifecycle events;
2. resolve the canonical public URL of changed content;
3. submit eligible URLs to the IndexNow API safely;
4. expose normalized submission status for administration, Hub and future tooling.

The plugin must avoid knowing the internal SQL schema, routing rules or business logic of third-party content plugins whenever Geeklog interoperability APIs can provide the required information.

This roadmap follows the current recommendations in the `hostellerie/memorandum` repository, especially:

- `plugin-content-interoperability-contract.md`;
- `seo-measurement-interoperability-guide.md`;
- `plugin-api-reference-2.2.2.md`.

---

## Compatibility target

IndexNow 1.3.0 must preserve the current modernization compatibility target:

- **Geeklog 2.1.1 through 2.2.2**;
- **PHP 5.6 through PHP 8.1**.

New code must therefore use the common safe subset of these versions where practical.

Geeklog 2.2.2 capabilities should be used when available, while safe fallbacks remain available for Geeklog 2.1.1.

Compatibility fallbacks should be clearly identified as legacy support rather than remain the primary architecture.

---

# 1. Architectural principles

## 1.1 Keep one clear responsibility

IndexNow should remain responsible for **announcing content changes**.

It should not become a replacement for:

- Google Search Console;
- Bing Webmaster reporting;
- Google Analytics;
- Hub orchestration;
- SEO scoring;
- content recommendations;
- plugin-specific indexing dashboards.

Preferred separation:

```text
Content plugin = owns content, permissions and canonical URL
IndexNow       = announces changed/deleted URLs
Hub            = consolidates state and recommendations
Analytics      = measures audience
Search Console = measures Google Search visibility
Bing Webmaster = measures Bing Search/indexing state
```

---

## 1.2 Prefer generic interoperability over plugin-specific logic

The preferred resolution path for plugin-owned content is:

```text
PLG_itemSaved() / PLG_itemDeleted()
        ↓
IndexNow lifecycle listener
        ↓
PLG_getItemInfo() and/or plugin_idToURL_*()
        ↓
canonical public URL
        ↓
IndexNow transport
```

IndexNow should not add plugin-specific SQL, routing or URL-building logic for Maps, Videos, Documents, Store or similar extensions.

A future content plugin should ideally become compatible with IndexNow without requiring any IndexNow code changes.

---

## 1.3 Keep strict validation at the transport boundary

Even URLs returned by Geeklog or another plugin must remain untrusted input until validated.

The existing protections should be preserved:

- HTTP/HTTPS only;
- same Geeklog host;
- matching port;
- no embedded credentials;
- control-character rejection;
- strict IndexNow key validation;
- TLS peer and host verification.

---

# 2. P1 — Core architecture for 1.3.0

These items define the minimum architecture expected before the 1.3.0 release.

## 2.1 Introduce one generic public URL resolver

Create a single primary resolver, conceptually:

```php
indexnow_resolve_public_url($type, $id, $subType = '', $options = array())
```

Responsibilities:

- ask the owning plugin for a public canonical URL;
- preserve anonymous-access semantics;
- support subtype-aware calls where available;
- normalize return values;
- isolate Geeklog 2.1.1 / 2.2.2 compatibility details;
- fall back to legacy core handling only when required.

Preferred plugin-owned save resolution:

```text
PLG_getItemInfo(type, id, 'url', uid=1, options)
        ↓
URL found → continue
        ↓
no URL → content is unresolved or not anonymously accessible
```

For delete-oriented deterministic URL resolution, `plugin_idToURL_*()` may be used where appropriate and safe.

---

## 2.2 Reduce `plugin_itemsaved_indexnow()` to orchestration

The lifecycle callback should progressively become a small controller.

Target responsibilities:

```text
build context
resolve URL
check previous public URL when needed
validate
submit
record result
```

The callback should not contain duplicated routing or permission logic for each supported content type unless required as a legacy fallback.

---

## 2.3 Preserve legacy core fallbacks without making them the main design

Existing Article, Static Pages and Topic compatibility must not be removed blindly.

For Geeklog versions where generic APIs do not provide enough information, preserve safe fallbacks for:

- article visibility;
- static page visibility;
- topic visibility;
- core URL construction.

These fallbacks should be isolated behind the generic resolver instead of being spread across lifecycle, cleanup and scheduled-task code.

Target model:

```text
1. generic Geeklog/plugin API
2. supported native URL callback
3. legacy core fallback
```

---

## 2.4 Normalize IndexNow submission status

Add a stable public helper, conceptually:

```php
indexnow_get_submission_status($type, $id, $subType = '')
```

Recommended normalized result:

```php
array(
    'provider'     => 'indexnow',
    'type'         => $type,
    'id'           => $id,
    'subtype'      => $subType,
    'event'        => '',
    'url'          => '',
    'submitted_at' => 0,
    'status'       => 'unknown',
    'http_code'    => 0,
    'message'      => ''
);
```

Recommended normalized statuses:

```text
success
failed
not-configured
not-resolved
queued
unknown
```

The existing database statuses do not need to be rewritten immediately. The public helper may translate legacy values such as `skipped` into a more precise normalized status.

This helper should allow Hub and administration tools to read IndexNow state without querying IndexNow tables directly or parsing logs.

---

## 2.5 Preserve submission history as the deletion safety authority

For deletion events, a previously successful public submission remains the safest proof that a URL should be resubmitted after deletion.

Preferred delete logic:

```text
previous successful public URL exists
        ↓ yes
submit previous URL

        ↓ no
record skipped/not-resolved
```

IndexNow should not guess that an arbitrary deleted content URL had previously been public.

The existing historical safety principle should therefore be retained.

---

## 2.6 Handle canonical URL changes explicitly

When a saved item resolves to a canonical URL different from the last successful public URL, IndexNow should recognize the transition.

Example:

```text
previous URL: https://example.org/old-url
current URL:  https://example.org/new-url
```

Target behavior:

- submit the current canonical URL;
- schedule or submit the previous public URL for recrawl/remediation when appropriate;
- record both outcomes clearly in history;
- avoid duplicate submissions when the URLs are identical.

This should reduce reliance on later cleanup audits to discover ordinary canonical changes.

---

## 2.7 Clarify the transport API

Introduce clearer internal names such as:

```php
indexnow_submit_url()
indexnow_submit_urls()
```

The existing `send_to_indexnow()` may remain temporarily as a compatibility wrapper if needed.

Transport functions should own:

- URL validation;
- key validation;
- key location construction;
- GET/single submission;
- POST/batch submission;
- HTTP result handling;
- history recording hooks;
- safe logging.

Lifecycle callbacks should not duplicate transport checks.

---

# 3. P2 — Simplification and cleanup

These changes should be completed in 1.3.0 where low-risk, or deferred only if they threaten backward compatibility.

## 3.1 Remove duplicated URL builders

Article, Static Pages and Topic URL construction currently appears in more than one logical area.

Move all core fallback URL construction behind one helper.

The cleanup subsystem, lifecycle handlers and scheduled task should reuse the same resolver wherever practical.

---

## 3.2 Review `indexnow_resolve_deleted_plugin_url()`

Deletion submission currently relies primarily on historical public URLs for safety.

Review whether `indexnow_resolve_deleted_plugin_url()` still has a justified runtime role.

Possible outcomes:

- retain it only for diagnostics/audits;
- use it only for deterministic canonical comparison;
- simplify it;
- remove it if history remains the sole deletion authority.

Avoid maintaining two competing deletion-resolution models.

---

## 3.3 Simplify cleanup responsibilities

`cleanup.php` should focus on:

```text
audit
queue remediation
process remediation
report remediation state
```

It should reuse common URL-resolution helpers rather than maintain separate core URL logic.

Review whether pseudo-records such as:

```text
__audit__
__legacy__
```

should remain in the cleanup queue table.

The cleanup table should preferably describe actual remediation work rather than mix operational URLs and audit metadata.

If audit summary state is still needed, prefer lightweight logging or derived statistics before introducing another table.

---

## 3.4 Make schema repair an upgrade responsibility

Runtime history reads should not normally trigger database migrations.

Target model:

```text
install / upgrade → create or repair schema
runtime            → verify and use schema
```

Review `indexnow_history_table_ready()` so it primarily reports readiness instead of calling an updater during ordinary history access.

Migration and repair logic should remain idempotent in `install_updates.php` / plugin upgrade paths.

---

## 3.5 Clarify `submitted` versus `status`

The submission-history schema currently stores both:

```text
submitted
status
```

Document their distinct meanings:

```text
submitted = an HTTP transport attempt was made
status    = the outcome of the operation
```

Examples:

```text
submitted=0, status=skipped   → no request attempted
submitted=1, status=failed    → request attempted but failed
submitted=1, status=success   → request accepted
```

Do not remove the field unless a later schema migration proves it redundant.

---

## 3.6 Move environment detection out of admin rendering

The XMLSitemap coexistence/conflict detection is useful beyond `admin/index.php`.

Move it to a reusable helper such as:

```php
indexnow_get_environment_status()
```

or:

```php
indexnow_detect_native_support()
```

Potential consumers:

- IndexNow administration;
- Hub;
- diagnostics;
- future connectors;
- automated compatibility reports.

The plugin should continue avoiding duplicate IndexNow behavior when Geeklog/XMLSitemap already provides enabled IndexNow support.

---

## 3.7 Reduce the size of `admin/index.php`

The current administration controller mixes access control, environment detection, list rendering, filtering, actions and presentation.

Progressively extract reusable administration helpers without over-engineering the plugin.

A conservative target could be:

```text
functions.inc
submission_history.php
cleanup.php
admin/functions.php
admin/index.php
```

Do not introduce a large framework or class hierarchy solely for cosmetic separation.

---

# 4. P3 — Interoperability extensions

These capabilities are desirable but should not block 1.3.0 if the broader Geeklog plugin ecosystem is not ready.

## 4.1 Generic collection support for scheduled submissions

The current scheduled task remains strongly oriented toward core Stories and Static Pages.

Longer term, IndexNow should be able to consume content collections exposed by compatible plugins through the common Item Info convention:

```php
PLG_getItemInfo(
    $type,
    '*',
    'id,url,date-modified',
    1,
    array(
        'since' => $timestamp,
        'limit' => 100,
        'order' => 'modified-desc'
    )
);
```

This should only be introduced once enough content plugins implement collection support consistently.

For 1.3.0, the existing core scheduled fallback may remain.

---

## 4.2 Capability discovery

Add a small, dependency-free capability description, conceptually:

```php
indexnow_get_capabilities()
```

Possible initial result:

```php
array(
    'submission'        => true,
    'batch_submission'  => true,
    'submission_status' => true,
    'lifecycle_saved'   => true,
    'lifecycle_deleted' => true,
    'cleanup'           => true
);
```

This is initially a plugin convention rather than a claim that Geeklog core already provides a universal capability dispatcher.

The goal is to let Hub, diagnostics and future connectors detect supported integration points without hard-coded version assumptions.

---

# 5. Explicit non-goals for 1.3.0

Do **not** add the following to IndexNow 1.3.0:

- Google Search Console reporting;
- Bing Webmaster reporting;
- Analytics reporting;
- SEO scoring;
- content-quality recommendations;
- Hub relationship logic;
- plugin-specific SQL for third-party content plugins;
- plugin-specific routing for Maps, Videos, Documents, Store, etc.;
- a large generic service framework;
- a new persistence table unless a demonstrated requirement cannot be met otherwise.

---

# 6. Queue strategy

Do not introduce a full general-purpose asynchronous queue in 1.3.0 unless real-world testing proves it necessary.

Preferred 1.3.0 behavior:

```text
normal save/delete → immediate submission
manual batch       → batch submission
scheduled content  → scheduled task
cleanup remediation→ scheduled task
```

A general queue may be considered for a later release if sites with high event volume, rate limiting or unreliable network conditions justify the additional state and complexity.

---

# 7. Suggested internal structure

The target is functional separation, not a forced rewrite into classes.

Conceptual layout:

```text
functions.inc
│
├─ lifecycle
│   ├─ plugin_itemsaved_indexnow()
│   └─ plugin_itemdeleted_indexnow()
│
├─ resolver
│   └─ indexnow_resolve_public_url()
│
├─ transport
│   ├─ indexnow_validate_url()
│   ├─ indexnow_validate_key()
│   ├─ indexnow_submit_url()
│   └─ indexnow_submit_urls()
│
├─ environment
│   └─ indexnow_get_environment_status()
│
└─ scheduled task

submission_history.php
    persistence + normalized status access

cleanup.php
    audit + remediation only

admin/index.php
    administration controller/rendering
```

This structure may remain split across the current files where that minimizes risk. The important goal is separation of responsibility and removal of duplicated logic.

---

# 8. Migration and backward-compatibility rules

## 8.1 Existing installations

Upgrading from 1.2.x must preserve:

- configured IndexNow key;
- history retention configuration;
- submission history;
- cleanup queue state;
- successful historical URLs used for deletion safety.

Do not discard history simply to simplify the schema.

---

## 8.2 Database changes

Prefer no schema change unless required.

If a schema migration becomes necessary:

- it must be idempotent;
- it must work on Geeklog 2.1.1 and 2.2.2;
- upgrade must not depend on an external HTTP request;
- failed migrations must fail safely;
- historical security information must not be silently lost.

---

## 8.3 PHP compatibility

Avoid syntax or APIs unavailable in PHP 5.6 for the 1.3.0 compatibility target.

Do not introduce type declarations, scalar return types, null coalescing, arrow functions or other syntax that would make the plugin unparsable on PHP 5.6.

---

# 9. Testing matrix

Minimum release testing should cover:

| Area | Geeklog 2.1.1 | Geeklog 2.2.2 |
|---|---:|---:|
| PHP 5.6 compatibility | Required | N/A |
| PHP 8.1 compatibility | Recommended where installable | Required |
| Article save/update | Required | Required |
| Article public → private | Required | Required |
| Article delete | Required | Required |
| Static Page save/delete | Required | Required |
| Topic save/delete | Required | Required |
| Plugin-owned item via Item Info | Required fallback behavior | Required |
| Namespaced ID such as `marker:123` | Required | Required |
| `sub_type` lifecycle path | Legacy-safe | Required |
| Canonical URL change | Required | Required |
| Invalid/foreign URL rejection | Required | Required |
| Missing/invalid key | Required | Required |
| Single submission | Required | Required |
| Batch submission | Required | Required |
| Scheduled task | Required | Required |
| Cleanup remediation | Required | Required |
| XMLSitemap coexistence detection | Required | Required |
| Upgrade from 1.2.x | Required | Required |

Third-party interoperability tests should include at least one plugin implementing `plugin_getiteminfo_*()` correctly.

---

# 10. Documentation tasks

Before release:

- update README version references from 1.2.1 to 1.3.0;
- document the generic lifecycle/resolution model;
- document normalized submission status;
- document canonical URL change behavior;
- document legacy fallbacks clearly;
- document coexistence with XMLSitemap IndexNow support;
- update CHANGELOG;
- ensure installation/upgrade instructions remain accurate;
- document any new public helper intended for Hub or other plugins.

---

# 11. Recommended implementation order

## Phase 1 — Resolver and lifecycle

- [ ] Add generic public URL resolver.
- [ ] Centralize Article / Static Pages / Topic fallback handling.
- [ ] Simplify `plugin_itemsaved_indexnow()`.
- [ ] Review and simplify delete resolution.
- [ ] Add canonical URL change handling.

## Phase 2 — Status and transport

- [ ] Add normalized submission-status helper.
- [ ] Introduce clear single/batch transport helpers.
- [ ] Preserve strict URL/key/cURL security controls.
- [ ] Document `submitted` versus `status` semantics.

## Phase 3 — Cleanup and runtime simplification

- [ ] Reuse common resolver in cleanup.
- [ ] Review `__audit__` and `__legacy__` pseudo-records.
- [ ] Remove runtime-triggered migration behavior from history readiness checks where safe.
- [ ] Move XMLSitemap/environment detection to reusable plugin code.

## Phase 4 — Administration and interoperability

- [ ] Reduce `admin/index.php` responsibilities.
- [ ] Add lightweight capability discovery.
- [ ] Evaluate generic collection support for scheduled submissions.

## Phase 5 — Release validation

- [ ] Run Geeklog 2.1.1 / PHP 5.6 tests.
- [ ] Run Geeklog 2.2.2 / PHP 8.1 tests.
- [ ] Verify 1.2.x upgrade path.
- [ ] Verify security cleanup preservation.
- [ ] Update README / CHANGELOG / INSTALL.
- [ ] Build installable archive and checksum.

---

# 12. Definition of done for IndexNow 1.3.0

IndexNow 1.3.0 is ready when:

1. lifecycle handling is centered on generic Geeklog interoperability;
2. plugin-owned content can be resolved without IndexNow knowing plugin SQL or routing;
3. legacy Geeklog core compatibility remains functional;
4. canonical URL changes are handled predictably;
5. delete submissions remain history-safe;
6. strict URL/key/transport validation is preserved;
7. IndexNow exposes a normalized submission state for consumers;
8. cleanup reuses common resolution logic where practical;
9. ordinary runtime reads no longer perform unexpected schema migrations where avoidable;
10. XMLSitemap coexistence remains detectable;
11. no unrelated Search Console, Analytics, Bing Webmaster or Hub logic is added;
12. Geeklog 2.1.1–2.2.2 and PHP 5.6–8.1 compatibility is validated according to the testing matrix.

---

## Future work after 1.3.0

Possible later directions, only if justified by real usage:

- generic collection discovery across compatible content plugins;
- optional asynchronous submission queue;
- richer capability discovery shared with Hub/connectors;
- standardized URL-to-item interoperability when the ecosystem adopts it;
- richer submission diagnostics and retry policies;
- removal of legacy core fallbacks once the supported Geeklog baseline no longer requires them.
