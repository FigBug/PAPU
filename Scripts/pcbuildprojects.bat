set MSBUILD_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\MSBuild\15.0\Bin\MSBuild.exe

set ROOT=%cd%
cd "%ROOT%"

cd Scripts
mkdir bin
mkdir bin\mac

cd "..\plugin\Builds\VisualStudio2017"
"%MSBUILD_EXE%" PAPU.sln /p:VisualStudioVersion=15.0 /m /t:Build /p:Configuration=Release /p:Platform=x64 /p:PreferredToolArchitecture=x64

