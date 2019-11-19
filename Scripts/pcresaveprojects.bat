set MSBUILD_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\MSBuild\15.0\Bin\MSBuild.exe

set ROOT=%cd%

cd "%ROOT%\modules\juce\extras\Projucer\Builds\VisualStudio2017\"
"%MSBUILD_EXE%" Projucer.sln /p:VisualStudioVersion=15.0 /m /t:Build /p:Configuration=Release /p:Platform=x64 /p:PreferredToolArchitecture=x64 

.\x64\Release\App\Projucer.exe --resave "%ROOT%\plugin\PAPU.jucer"

cd "%ROOT%"