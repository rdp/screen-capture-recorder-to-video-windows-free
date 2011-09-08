module WMI
  module Privilege
    # Required to create a primary token.
    CreateToken = 1

    # Required to assign the primary token of a process.
    PrimaryToken = 2

    # Required to lock physical pages in memory.
    LockMemory = 3

    # Required to increase the quota assigned to a process.
    IncreaseQuota = 4

    # Required to create a machine account.
    MachineAccount = 5

    # Identifies its holder as part of the trusted computer base. Some trusted, protected
    # subsystems are granted this privilege.
    Tcb = 6

    # Required to perform a number of security-related functions, such as controlling and
    # viewing audit messages. This privilege identifies its holder as a security operator.
    #
    Security = 7

    # Required to take ownership of an object without being granted discretionary access.
    # This privilege allows the owner value to be set only to those values that the holder
    # may legitimately assign as the owner of an object.
    TakeOwnership = 8

    # Required to load or unload a device driver.
    LoadDriver = 9

    # Required to gather profiling information for the entire system.
    SystemProfile = 10

    # Required to modify the system time.
    Systemtime = 11

    # Required to gather profiling information for a single process.
    ProfileSingleProcess = 12

    # Required to increase the base priority of a process.
    IncreaseBasePriority = 13

    # Required to create a paging file.
    CreatePagefile = 14

    # Required to create a permanent object.
    CreatePermanent = 15

    # Required to perform backup operations.
    Backup = 16

    # Required to perform restore operations. This privilege enables you to set any valid
    # user or group security identifier (SID) as the owner of an object.
    Restore = 17

    # Required to shut down a local system.
    Shutdown = 18

    # Required to debug a process.
    Debug = 19

    # Required to generate audit-log entries.
    Audit = 20

    # Required to modify the nonvolatile RAM of systems that use this type of memory to store
    # configuration information.
    SystemEnvironment = 21

    # Required to receive notifications of changes to files or directories. This privilege
    # also causes the system to skip all traversal access checks. It is enabled by default
    # for all users.
    ChangeNotify = 22

    # Required to shut down a system using a network request.
    RemoteShutdown = 23

    # Required to remove a computer from a docking station.
    Undock = 24

    # Required to synchronize directory service data.
    SyncAgent = 25

    # Required to enable computer and user accounts to be trusted for delegation.
    EnableDelegation = 26

    # Required to perform volume maintenance tasks. Windows 2000, Windows NT 4.0, and Windows
    # Me/98/95:  This privilege is not available.
    ManageVolume = 27
  end
end