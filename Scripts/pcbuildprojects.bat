set ROOT=%cd%
cd "%ROOT%"

cd Scripts
mkdir bin
mkdir bin/mac

cd "..\plugin\Builds\Windows"
xcodebuild -configuration Release || exit 1
"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe" PAPU.sln /p:VisualStudioVersion=15.0 /m /t:Build /p:Configuration=Release /p:Platform=x64 /p:PreferredToolArchitecture=x64  /p:TreatWarningsAsErrors=true

