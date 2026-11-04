param(
  [Parameter(Mandatory=$true)][string]$Dll,
  [Parameter(Mandatory=$true)][string]$OutputLib,
  [Parameter(Mandatory=$true)][string]$LibExe
)
$ErrorActionPreference = 'Stop'
$b = [IO.File]::ReadAllBytes((Resolve-Path $Dll))
function U16([int]$p) { [BitConverter]::ToUInt16($b,$p) }
function U32([int]$p) { [BitConverter]::ToUInt32($b,$p) }
$pe = [int](U32 0x3c)
if ((U32 $pe) -ne 0x4550 -or (U16 ($pe+4)) -ne 0x8664) { throw "Expected x64 PE DLL: $Dll" }
$optional = $pe+24
if ((U16 $optional) -ne 0x20b) { throw "Expected PE32+ DLL: $Dll" }
$sections = $optional + (U16 ($pe+20))
function Offset([uint32]$rva) {
  for ($i=0; $i -lt (U16 ($pe+6)); $i++) {
    $s=$sections+40*$i; $start=U32 ($s+12)
    $length=[Math]::Max((U32 ($s+8)),(U32 ($s+16)))
    if ($rva -ge $start -and $rva -lt ($start+$length)) {
      return [int]($rva-$start+(U32 ($s+20)))
    }
  }
  throw "RVA outside sections: $rva"
}
function CString([int]$p) {
  $end=$p
  while ($b[$end] -ne 0) { $end++ }
  [Text.Encoding]::ASCII.GetString($b,$p,$end-$p)
}
$exportRva=U32 ($optional+112)
if ($exportRva -eq 0) { throw "No DLL exports: $Dll" }
$export=Offset $exportRva
$count=U32 ($export+24)
$names=Offset (U32 ($export+32))
$ordinals=Offset (U32 ($export+36))
$functions=Offset (U32 ($export+28))
$lines=@()
$lines += ('LIBRARY "'+[IO.Path]::GetFileName($Dll)+'"')
$lines += 'EXPORTS'
for ($i=0; $i -lt $count; $i++) {
  $name=CString (Offset (U32 ($names+4*$i)))
  if ($name -notmatch '^[A-Za-z_?@$][A-Za-z0-9_?@$]*$') { throw "Unexpected export name: $name" }
  $ordinal=U16 ($ordinals+2*$i)
  $functionRva=U32 ($functions+4*$ordinal)
  # GLEW exports data pointers as well as functions. Preserve the DATA flag.
  $data=$false
  for ($j=0; $j -lt (U16 ($pe+6)); $j++) {
    $s=$sections+40*$j; $start=U32 ($s+12)
    $length=[Math]::Max((U32 ($s+8)),(U32 ($s+16)))
    if ($functionRva -ge $start -and $functionRva -lt ($start+$length)) {
      $data=((U32 ($s+36)) -band 0x20000000) -eq 0
      break
    }
  }
  $lines += ('  '+$name+$(if ($data) {' DATA'} else {''}))
}
$parent=Split-Path -Parent $OutputLib
New-Item -ItemType Directory -Force -Path $parent | Out-Null
$def=[IO.Path]::ChangeExtension($OutputLib,'.def')
[IO.File]::WriteAllLines($def,[string[]]$lines,[Text.Encoding]::ASCII)
& $LibExe /nologo /machine:x64 "/def:$def" "/out:$OutputLib"
if ($LASTEXITCODE -ne 0) { throw "lib.exe failed ($LASTEXITCODE): $Dll" }
