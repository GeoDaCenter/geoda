cd reactgeoda
git submodule update --init --recursive
yarn
cd library
yarn
yarn build
cp out/index.html ../../CommonDistFiles
cp out/bundle.js ../../CommonDistFiles
cp out/bundle.js.map ../../CommonDistFiles