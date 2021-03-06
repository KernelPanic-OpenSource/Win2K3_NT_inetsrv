

<%@ Page %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<html>
	<HEAD>
		<META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=Windows-1252">
		<META HTTP-EQUIV="MSThemeCompatible" CONTENT="Yes">
		<META NAME="MS.LOCALE" CONTENT="EN-US">
		<!-- #include file = "publish.header.htm" -->
	</head>
	<body marginwidth="0" marginheight="0" LEFTMARGIN="0" TOPMARGIN="0" rightmargin="0" ONLOAD="BringToFront()">
		<!-- #include file = "publish.heading.htm" -->
		<table class="content" width="100%" cellpadding="8">
			<tr>
				<td>
					<h1><img src="..\..\images\business.gif" height="16" width="16" alt="Provider"> Provider - Details</h1>
					Use the <b>Details</b> tab to manage the name and descriptions of this 
					provider.
					<UL>
						<li>
							<b>Owner:</b> Displays the name of the user that owns this provider.
						</li>
						<li>
							<b>Provider Key:</b> Displays the unique key that is assigned to this provider which is used during programmatic queries.
						</li>
						<li>
							<b>Name:</b> Lists the names of this provider. If more than one name is provided, the default name appears at the top of the list.
							<ul>
								<li class="action">
									Click <b>Add Name</b> to add a name in a different language.
								</li>
								<li class="action">
									Click <b>Edit</b> to modify a name.
								</li>
								<li class="action">
									Click <b>Delete</b> to delete a name.
								</li>
							</ul>
						</li>
						<li>
							<b>Description:</b> Lists descriptions for this provider and the language 
							for which each description is written.
						</li>
						<ul>
							<li class="action">
								Click <b>Add Description</b> to add a description.
							</li>
							<li class="action">
								Click <b>Edit</b> to modify a description.
							</li>
							<li class="action">
								Click <b>Delete</b> to delete a description.
							</li>
						</ul>
					</UL>
					<h3>More Information</h3>
					<!-- #include file = "glossary.provider.htm" -->
				</td>
			</tr>
		</table>
		<!-- #include file = "publish.footer.htm" -->
	</body>
</html>

 