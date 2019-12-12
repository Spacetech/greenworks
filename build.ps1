Param (
    [Parameter(Mandatory = $True)][string]$version,
    [Switch] $OnlyWindows,
    [Switch] $Only64Bit
)

$ErrorActionPreference = "Continue"

$root = [System.IO.Path]::GetFullPath($PSScriptRoot)

$bashPath = (($root -replace "\\","/") -replace ":","").ToLower().Trim("/")

Remove-Item $root\lib -Force -Recurse -ErrorAction SilentlyContinue

# npm install -g node-gyp

npm install --ignore-scripts

if (-not $Only64Bit) {
    node-gyp rebuild --target=$version --arch=ia32 --dist-url=https://atom.io/download/electron
}

node-gyp rebuild --target=$version --arch=x64 --dist-url=https://atom.io/download/electron

if (-not $OnlyWindows)
{
    if (-not $Only64Bit)
    {
        bash -c "cd /mnt/$bashPath && HOME=~/.electron-gyp node-gyp rebuild --target=$version --arch=ia32 --dist-url=https://atom.io/download/electron"
    }

    bash -c "cd /mnt/$bashPath && HOME=~/.electron-gyp node-gyp rebuild --target=$version --arch=x64 --dist-url=https://atom.io/download/electron"
}
