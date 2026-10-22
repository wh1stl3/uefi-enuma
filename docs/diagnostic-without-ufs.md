# GOP isolation without OEM UFSDxe

The default configuration has `ENUMA_DIAG_DISABLE_UFS=0` and keeps the native
driver. A diagnostic build may set it to 1 to omit UFSDxe from Components,
the firmware volume and APRIORI. The unchanged private input set is validated
before its generated include files are patched in device out/.

This mode isolates later display and platform execution after a GUID-qualified
probe reached UFSDxe's entry call but no post-return gate was observed. It does
not fix UFS and does not establish storage or complete platform readiness.
No protocol stub is substituted. The ordinary firmware configuration remains
complete and the source binary is preserved for the storage investigation.

The device wrapper records the exact omission in the completed build report.
Its FD auditor permits only this single explicitly declared omission, requires
the report to match the actual FD digest, rejects the module or its APRIORI GUID
if still present, and byte-checks every remaining OEM payload. A missing driver
in a normal build remains an error.
