Param (
    [Parameter(Mandatory = $True)][string]$version
)

$ErrorActionPreference = "Stop"

$root = [System.IO.Path]::GetFullPath($PSScriptRoot)

Remove-Item $root\lib -Force -Recurse -ErrorAction SilentlyContinue

node-gyp rebuild --target=$version --arch=ia32 --dist-url=https://atom.io/download/electron
git
node-gyp rebuild --target=$version --arch=x64 --dist-url=https://atom.io/download/electron

bash -c "cd /mnt/d/Programming/greenworks && HOME=~/.electron-gyp node-gyp rebuild --target=$version --arch=ia32 --dist-url=https://atom.io/download/electron"

bash -c "cd /mnt/d/Programming/greenworks && HOME=~/.electron-gyp node-gyp rebuild --target=$version --arch=x64 --dist-url=https://atom.io/download/electron"
