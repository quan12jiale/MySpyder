config="RelWithDebInfo"
pathToBuild="..\..\build\Sln\x64\RelWithDebInfo"
cmake -B${pathToBuild%} -D CMAKE_CONFIGURATION_TYPES=${config}
