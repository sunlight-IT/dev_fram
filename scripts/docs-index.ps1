[CmdletBinding()]
param(
  [switch]$Update,
  [switch]$Check
)

$ErrorActionPreference = "Stop"
if (($Update -and $Check) -or (-not $Update -and -not $Check))
{
  throw "Specify exactly one of -Update or -Check."
}
$mode = if ($Update) { "update" } else { "check" }
& python (Join-Path $PSScriptRoot "docs-library.py") $mode
exit $LASTEXITCODE
