# Installer container failure, v50 / 24A437

A hardware breakpoint at 0x1ab0b4468 + cache slide 0x121b0000 stops after
MIMCMContainer's daemon-container request. x20 (container result) is nil;
x0 is the NSError. The error has domain MIInstallerErrorDomain and code 4.
Its user-info description decodes to:

    UserManager returned an empty persona list

The FunctionName value is:

    -[MIUserManagement _onQueue_refreshPersonaInformationWithError:]

The underlying NSError has code 2. Thus the observed ENOENT is about the
empty persona response; it does not establish a missing filesystem path.
The breakpoint was deleted and debugger detached, then the installer job
was removed (errno 0). No error/result object was modified.
