cd scripts
cd bindgen
clang++ -Xclang -ast-dump=json -fsyntax-only -Xclang -ast-dump-filter=breeze::js -std=c++2c ..\..\src\shell\script\binding_types.h > .\ast.json
yarn && yarn gen
echo Done