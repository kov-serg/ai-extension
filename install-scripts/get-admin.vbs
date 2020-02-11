if WScript.Arguments.Count>0 then
 CreateObject("Shell.Application").ShellExecute WScript.Arguments(0),,,"runas",1
else
 WScript.Echo("Usage: get-admin command")
end if
