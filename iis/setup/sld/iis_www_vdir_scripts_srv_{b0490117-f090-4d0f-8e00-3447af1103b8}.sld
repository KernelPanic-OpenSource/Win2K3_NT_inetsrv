<?xml version="1.0" encoding="UTF-16"?>
<!DOCTYPE DCARRIER SYSTEM "Mantis.DTD">

  <DCARRIER
    CarrierRevision="1"
    DTDRevision="16"
  >
    <TASKS
      Context="1"
      PlatformGUID="{00000000-0000-0000-0000-000000000000}"
    >    </TASKS>

    <PLATFORMS
      Context="1"
    >    </PLATFORMS>

    <REPOSITORIES
      Context="1"
      PlatformGUID="{00000000-0000-0000-0000-000000000000}"
    >    </REPOSITORIES>

    <GROUPS
      Context="1"
      PlatformGUID="{00000000-0000-0000-0000-000000000000}"
    >    </GROUPS>

    <COMPONENTS
      Context="1"
      PlatformGUID="{00000000-0000-0000-0000-000000000000}"
    >
      <COMPONENT
        ComponentVSGUID="{3296F3F1-334F-48F7-8E83-98C8951A42A7}"
        ComponentVIGUID="{AE67B8C8-BE4F-4488-AE8D-0685C42FDCCE}"
        Revision="4"
        RepositoryVSGUID="{8E0BE9ED-7649-47F3-810B-232D36C430B4}"
        Visibility="1000"
        MultiInstance="False"
        Released="True"
        Editable="True"
        HTMLFinal="False"
        IsMacro="False"
        Opaque="False"
        Context="1"
        PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
      >
        <PROPERTIES
          Context="1"
          PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
        >        </PROPERTIES>

        <RESOURCES
          Context="1"
          PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
        >
          <RESOURCE
            Name="FBOCMgr(819):&quot;%11%\setup\iis.dll&quot;,&quot;OcEntry&quot;,&quot;iis_www_vdir_scripts&quot;,&quot;iis_www_parent&quot;"
            ResTypeVSGUID="{ECE0991C-AF0A-4CCB-8C01-354CD5040638}"
            BuildTypeMask="819"
            BuildOrder="1000"
            Localize="False"
            Disabled="False"
            Context="1"
            PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
          >
            <PROPERTIES
              Context="1"
              PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
            >
              <PROPERTY
                Name="ComponentVSGUID"
                Format="GUID"
                Context="1"
                PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
              >{00000000-0000-0000-0000-000000000000}</PROPERTY>

              <PROPERTY
                Name="DLLEntryPoint"
                Format="String"
                Context="1"
                PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
              >OcEntry</PROPERTY>

              <PROPERTY
                Name="ErrorControl"
                Format="Integer"
                Context="1"
                PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
              >0</PROPERTY>

              <PROPERTY
                Name="FilePath"
                Format="String"
                Context="1"
                PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
              >%11%\setup\iis.dll</PROPERTY>

              <PROPERTY
                Name="GroupName"
                Format="String"
                Context="1"
                PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
              >iis_www_parent</PROPERTY>

              <PROPERTY
                Name="INFName"
                Format="String"
                Context="1"
                PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
              >iis.inf</PROPERTY>

              <PROPERTY
                Name="OCName"
                Format="String"
                Context="1"
                PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
              >iis_www_vdir_scripts</PROPERTY>

              <PROPERTY
                Name="Reboot"
                Format="Boolean"
                Context="1"
                PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
              >False</PROPERTY>

              <PROPERTY
                Name="Start"
                Format="Integer"
                Context="1"
                PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
              >1</PROPERTY>

              <PROPERTY
                Name="Timeout"
                Format="Integer"
                Context="1"
                PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
              >0</PROPERTY>

              <PROPERTY
                Name="Type"
                Format="Integer"
                Context="1"
                PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
              >2</PROPERTY>
            </PROPERTIES>

            <DISPLAYNAME>FBOCMgr(819):&quot;&quot;,&quot;&quot;,&quot;&quot;,&quot;&quot;</DISPLAYNAME>

            <DESCRIPTION></DESCRIPTION>
          </RESOURCE>
        </RESOURCES>

        <GROUPMEMBERS
        >
          <GROUPMEMBER
            GroupVSGUID="{E01B4103-3883-4FE8-992F-10566E7B796C}"
          ></GROUPMEMBER>

          <GROUPMEMBER
            GroupVSGUID="{D2AF69C5-EE0A-4CD3-9BA4-63BB2ED7732C}"
          ></GROUPMEMBER>
        </GROUPMEMBERS>

        <DEPENDENCIES
          Context="1"
          PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
        >
          <DEPENDENCY
            Class="Include"
            Type="All"
            DependOnGUID="{604ED056-D700-426E-B4F1-C5CECB1AE8D7}"
            MinRevision="0"
            Disabled="False"
            Context="1"
            PlatformGUID="{B784E719-C196-4DDB-B358-D9254426C38D}"
          >
            <PROPERTIES
              Context="1"
              PlatformGUID="{00000000-0000-0000-0000-000000000000}"
            >            </PROPERTIES>
          </DEPENDENCY>
        </DEPENDENCIES>

        <DISPLAYNAME>IIS www vdir scripts srv</DISPLAYNAME>

        <VERSION>1.0</VERSION>

        <DESCRIPTION>IIS web virtual directory for scripts</DESCRIPTION>

        <COPYRIGHT>2000 Microsoft Corp.</COPYRIGHT>

        <VENDOR>Microsoft Corp.</VENDOR>

        <OWNERS>cachille</OWNERS>

        <AUTHORS>aaronl; cachille</AUTHORS>

        <DATECREATED>4/5/2001</DATECREATED>

        <DATEREVISED>8/9/2001 9:46:13 PM</DATEREVISED>
      </COMPONENT>
    </COMPONENTS>

    <RESTYPES
      Context="1"
      PlatformGUID="{00000000-0000-0000-0000-000000000000}"
    >    </RESTYPES>
  </DCARRIER>
