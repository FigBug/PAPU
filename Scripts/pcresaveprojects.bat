set ROOT=%cd%

cd "%ROOT%\modules\juce\extras\Projucer\Builds\VisualStudio2017\"
"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe" Projucer.sln /p:VisualStudioVersion=15.0 /m /t:Build /p:Configuration=Release /p:Platform=x64 /p:PreferredToolArchitecture=x64  /p:TreatWarningsAsErrors=true

.\build\Release\Projucer.exe --resave "%ROOT%\plugin\PAPU.jucer"

cd "%ROOT%"