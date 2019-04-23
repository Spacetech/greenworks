npm install --ignore-scripts

node-gyp rebuild --target=$1 --arch=x64 --dist-url=https://atom.io/download/electron
