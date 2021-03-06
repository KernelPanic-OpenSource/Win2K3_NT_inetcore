<%
'/=======================================================>
'/            Signup Server Sample Page 07
'/                  Daniel Jacobson
'/			  Modified by Edward C Kubaitis
'/                  Rev .07    9/98
'/=======================================================>
'/=======================================================>
'/ Get all of the varriables from the ICW 
'/and get them set into varriables
'/=======================================================>
	USER_COMPANYNAME = Request("USER_COMPANYNAME")
	USER_FIRSTNAME = Request("USER_FIRSTNAME")
	USER_LASTNAME = Request("USER_LASTNAME")
	USER_ADDRESS = Request("USER_ADDRESS")
	USER_MOREADDRESS = Request("USER_MOREADDRESS")
	USER_CITY = Request("USER_CITY")
	USER_STATE = Request("USER_STATE")
	USER_ZIP = Request("USER_ZIP")
	USER_PHONE = Request("USER_PHONE")
	PAYMENT_TYPE = Request("PAYMENT_TYPE")
	PAYMENT_BILLNAME = Request("PAYMENT_BILLNAME")
	PAYMENT_BILLADDRESS = Request("PAYMENT_BILLADDRESS")
	PAYMENT_BILLEXADDRESS = Request("PAYMENT_BILLEXADDRESS")
	PAYMENT_BILLCITY = Request("PAYMENT_BILLCITY")
	PAYMENT_BILLSTATE = Request("PAYMENT_BILLSTATE")
	PAYMENT_BILLZIP = Request("PAYMENT_BILLZIP")
	PAYMENT_BILLPHONE = Request("PAYMENT_BILLPHONE")
	PAYMENT_DISPLAYNAME = Request("PAYMENT_DISPLAYNAME")
	PAYMENT_CARDNUMBER = Request("PAYMENT_CARDNUMBER")
	PAYMENT_EXMONTH = Request("PAYMENT_EXMONTH")
	PAYMENT_EXYEAR = Request("PAYMENT_EXYEAR")
	PAYMENT_CARDHOLDER = Request("PAYMENT_CARDHOLDER")
	BILLING =  Request("BILLING") 
	EMAILNAME = Request("EMAILNAME")
	EMAILPASSWORD = Request("EMAILPASSWORD")
	POPSELECTION = Request("POPSELECTION")

'/=======================================================>
'/ lowercase the contents of the varriables and 
'/ then trim any leading or trailing spaces
'/=======================================================>
	If USER_COMPANYNAME <> "" Then
		USER_COMPANYNAME = LCASE(USER_COMPANYNAME)
		USER_COMPANYNAME = TRIM(USER_COMPANYNAME)
	End If
	If USER_FIRSTNAME <> "" Then
		USER_FIRSTNAME = LCASE(USER_FIRSTNAME)
		USER_FIRSTNAME = TRIM(USER_FIRSTNAME)
	End If
	If USER_LASTNAME <> "" Then
		USER_LASTNAME = LCASE(USER_LASTNAME)
		USER_LASTNAME = TRIM(USER_LASTNAME)
	End If
	If USER_ADDRESS <> "" Then
		USER_ADDRESS = LCASE(USER_ADDRESS)
		USER_ADDRESS = TRIM(USER_ADDRESS)
	End If
	If USER_MOREADDRESS <> "" Then
		USER_MOREADDRESS = LCASE(USER_MOREADDRESS)
		USER_MOREADDRESS = TRIM(USER_MOREADDRESS)
	End If
	If USER_CITY <> "" Then
		USER_CITY = LCASE(USER_CITY)
		USER_CITY = TRIM(USER_CITY)
	End If
	If USER_STATE <> "" Then
		USER_STATE = LCASE(USER_STATE)
		USER_STATE = TRIM(USER_STATE)
	End If
	If USER_ZIP <> "" Then
		USER_ZIP = LCASE(USER_ZIP)
		USER_ZIP = TRIM(USER_ZIP)
	End If
	If USER_PHONE <> "" Then
		USER_PHONE = LCASE(USER_PHONE)
		USER_PHONE = TRIM(USER_PHONE)
	End If
	If PAYMENT_TYPE <> "" Then
		PAYMENT_TYPE = LCASE(PAYMENT_TYPE)
		PAYMENT_TYPE = TRIM(PAYMENT_TYPE)
	End If
	If PAYMENT_BILLNAME <> "" Then
		PAYMENT_BILLNAME = LCASE(PAYMENT_BILLNAME)
		PAYMENT_BILLNAME = TRIM(PAYMENT_BILLNAME)
	End If
	If PAYMENT_BILLADDRESS <> "" Then
		PAYMENT_BILLADDRESS = LCASE(PAYMENT_BILLADDRESS)
		PAYMENT_BILLADDRESS = TRIM(PAYMENT_BILLADDRESS)
	End If
	If PAYMENT_BILLEXADDRESS <> "" Then
		PAYMENT_BILLEXADDRESS = LCASE(PAYMENT_BILLEXADDRESS)
		PAYMENT_BILLEXADDRESS = TRIM(PAYMENT_BILLEXADDRESS)
	End If
	If PAYMENT_BILLCITY <> "" Then
		PAYMENT_BILLCITY = LCASE(PAYMENT_BILLCITY)
		PAYMENT_BILLCITY = TRIM(PAYMENT_BILLCITY)
	End If
	If PAYMENT_BILLSTATE <> "" Then
		PAYMENT_BILLSTATE = LCASE(PAYMENT_BILLSTATE)
		PAYMENT_BILLSTATE = TRIM(PAYMENT_BILLSTATE)
	End If
	If PAYMENT_BILLZIP <> "" Then
		PAYMENT_BILLZIP = LCASE(PAYMENT_BILLZIP)
		PAYMENT_BILLZIP = TRIM(PAYMENT_BILLZIP)
	End If
	If PAYMENT_BILLPHONE <> "" Then
		PAYMENT_BILLPHONE = LCASE(PAYMENT_BILLPHONE)
		PAYMENT_BILLPHONE = TRIM(PAYMENT_BILLPHONE)
	End If
	If PAYMENT_DISPLAYNAME <> "" Then
		PAYMENT_DISPLAYNAME = LCASE(PAYMENT_DISPLAYNAME)
		PAYMENT_DISPLAYNAME = TRIM(PAYMENT_DISPLAYNAME)
	End If
	If PAYMENT_CARDNUMBER <> "" Then
		PAYMENT_CARDNUMBER = LCASE(PAYMENT_CARDNUMBER)
		PAYMENT_CARDNUMBER = TRIM(PAYMENT_CARDNUMBER)
	End If
	If PAYMENT_EXMONTH <> "" Then
		PAYMENT_EXMONTH = LCASE(PAYMENT_EXMONTH)
		PAYMENT_EXMONTH = TRIM(PAYMENT_EXMONTH)
	End If
	If PAYMENT_EXYEAR <> "" Then
		PAYMENT_EXYEAR = LCASE(PAYMENT_EXYEAR)
		PAYMENT_EXYEAR = TRIM(PAYMENT_EXYEAR)
	End If
	If PAYMENT_CARDHOLDER <> "" Then
		PAYMENT_CARDHOLDER = LCASE(PAYMENT_CARDHOLDER)
		PAYMENT_CARDHOLDER = TRIM(PAYMENT_CARDHOLDER)
	End If
	If BILLING <> "" Then
		BILLING = LCASE(BILLING)
		BILLING = TRIM(BILLING)
	End If
	If EMAILNAME <> "" Then
		EMAILNAME = LCASE(EMAILNAME)
		EMAILNAME = TRIM(EMAILNAME)
	End If
	If EMAILPASSWORD <> "" Then
		EMAILPASSWORD = TRIM(EMAILPASSWORD)
	End If
	If POPSELECTION <> "" Then
		POPSELECTION = LCASE(POPSELECTION)
		POPSELECTION = TRIM(POPSELECTION)
	End If

'/=======================================================>
'/ Build the BACK BACKURL with the name value pairs 
'/=======================================================>
	BACKURL = "http://myserver/signup06.asp?USER_FIRSTNAME="
	BACKURL = BACKURL & Server.URLEncode(USER_FIRSTNAME)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "USER_COMPANYNAME="
	BACKURL = BACKURL & Server.URLEncode(USER_COMPANYNAME)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "USER_LASTNAME="
	BACKURL = BACKURL & Server.URLEncode(USER_LASTNAME)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "USER_ADDRESS="
	BACKURL = BACKURL & Server.URLEncode(USER_ADDRESS)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "USER_MOREADDRESS="
	BACKURL = BACKURL & Server.URLEncode(USER_MOREADDRESS)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "USER_CITY="
	BACKURL = BACKURL & Server.URLEncode(USER_CITY)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "USER_STATE="
	BACKURL = BACKURL & Server.URLEncode(USER_STATE)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "USER_ZIP="
	BACKURL = BACKURL & Server.URLEncode(USER_ZIP)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "USER_PHONE="
	BACKURL = BACKURL & Server.URLEncode(USER_PHONE)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "PAYMENT_TYPE="
	BACKURL = BACKURL & Server.URLEncode(PAYMENT_TYPE)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "PAYMENT_BILLNAME="
	BACKURL = BACKURL & Server.URLEncode(PAYMENT_BILLNAME)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "PAYMENT_BILLADDRESS="
	BACKURL = BACKURL & Server.URLEncode(PAYMENT_BILLADDRESS)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "PAYMENT_BILLEXADDRESS="
	BACKURL = BACKURL & Server.URLEncode(PAYMENT_BILLEXADDRESS)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "PAYMENT_BILLCITY="
	BACKURL = BACKURL & Server.URLEncode(PAYMENT_BILLCITY)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "PAYMENT_BILLSTATE="
	BACKURL = BACKURL & Server.URLEncode(PAYMENT_BILLSTATE)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "PAYMENT_BILLZIP="
	BACKURL = BACKURL & Server.URLEncode(PAYMENT_BILLZIP)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "PAYMENT_BILLPHONE="
	BACKURL = BACKURL & Server.URLEncode(PAYMENT_BILLPHONE)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "PAYMENT_DISPLAYNAME="
	BACKURL = BACKURL & Server.URLEncode(PAYMENT_DISPLAYNAME)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "PAYMENT_CARDNUMBER="
	BACKURL = BACKURL & Server.URLEncode(PAYMENT_CARDNUMBER)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "PAYMENT_EXMONTH="
	BACKURL = BACKURL & Server.URLEncode(PAYMENT_EXMONTH)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "PAYMENT_EXYEAR="
	BACKURL = BACKURL & Server.URLEncode(PAYMENT_EXYEAR)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "PAYMENT_CARDHOLDER="
	BACKURL = BACKURL & Server.URLEncode(PAYMENT_CARDHOLDER)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "BILLING="
	BACKURL = BACKURL & Server.URLEncode(BILLING)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "EMAILNAME="
	BACKURL = BACKURL & Server.URLEncode(EMAILNAME)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "EMAILPASSWORD="
	BACKURL = BACKURL & Server.URLEncode(EMAILPASSWORD)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "POPSELECTION="
	BACKURL = BACKURL & POPSELECTION

'/=======================================================>
'/ Build the NEXT BACKURL 
'/=======================================================>
	NEXTURL = "http://myserver/signup08.asp"

%>
<HTML>
<HEAD>
<TITLE>IEAK Sample</TITLE>
</HEAD>
<BODY  bgColor=THREEDFACE color=WINDOWTEXT><FONT style="font: 8pt ' ms sans serif' black">
<FORM NAME="PAGEID" ACTION="PAGE7" STYLE="background:transparent"></FORM>
<FORM NAME="BACK" ACTION="<% =BACKURL %>" STYLE="background:transparent"></FORM>
<FORM NAME="PAGETYPE" ACTION="FINISH" STYLE="background:transparent"></FORM>
<FORM NAME="NEXT" ACTION="<% =NEXTURL %>" STYLE="background:transparent">
<B>Welcome To IEAK Sample Signup Server</B><BR>
Your signup is completed. Please note that it may take us up to 30 minutes to process and activate your account. If your account is not active within 30 minutes please notify our system administrators and they will rectify the problem for you.<P>
<INPUT TYPE="HIDDEN" NAME="USER_COMPANYNAME" VALUE="<% =USER_COMPANYNAME %>">
<INPUT TYPE="HIDDEN" NAME="USER_FIRSTNAME" VALUE="<% =USER_FIRSTNAME %>">
<INPUT TYPE="HIDDEN" NAME="USER_LASTNAME" VALUE="<% =USER_LASTNAME %>">
<INPUT TYPE="HIDDEN" NAME="USER_ADDRESS" VALUE="<% =USER_ADDRESS %>">
<INPUT TYPE="HIDDEN" NAME="USER_MOREADDRESS" VALUE="<% =USER_MOREADDRESS %>">
<INPUT TYPE="HIDDEN" NAME="USER_CITY" VALUE="<% =USER_CITY %>">
<INPUT TYPE="HIDDEN" NAME="USER_STATE" VALUE="<% =USER_STATE %>">
<INPUT TYPE="HIDDEN" NAME="USER_ZIP" VALUE="<% =USER_ZIP %>">
<INPUT TYPE="HIDDEN" NAME="USER_PHONE" VALUE="<% =USER_PHONE %>">
<INPUT TYPE="HIDDEN" NAME="AREACODE" VALUE="<% =AREACODE %>">
<INPUT TYPE="HIDDEN" NAME="COUNTRYCODE" VALUE="<% =COUNTRYCODE %>">
<INPUT TYPE="HIDDEN" NAME="USER_FE_NAME" VALUE="<% =USER_FE_NAME %>">
<INPUT TYPE="HIDDEN" NAME="PAYMENT_TYPE" VALUE="<% =PAYMENT_TYPE %>">
<INPUT TYPE="HIDDEN" NAME="PAYMENT_BILLNAME" VALUE="<% =PAYMENT_BILLNAME %>">
<INPUT TYPE="HIDDEN" NAME="PAYMENT_BILLADDRESS" VALUE="<% =PAYMENT_BILLADDRESS %>">
<INPUT TYPE="HIDDEN" NAME="PAYMENT_BILLEXADDRESS" VALUE="<% =PAYMENT_BILLEXADDRESS %>">
<INPUT TYPE="HIDDEN" NAME="PAYMENT_BILLCITY" VALUE="<% =PAYMENT_BILLCITY %>">
<INPUT TYPE="HIDDEN" NAME="PAYMENT_BILLSTATE" VALUE="<% =PAYMENT_BILLSTATE %>">
<INPUT TYPE="HIDDEN" NAME="PAYMENT_BILLZIP" VALUE="<% =PAYMENT_BILLZIP %>">
<INPUT TYPE="HIDDEN" NAME="PAYMENT_BILLPHONE" VALUE="<% =PAYMENT_BILLPHONE %>">
<INPUT TYPE="HIDDEN" NAME="PAYMENT_DISPLAYNAME" VALUE="<% =PAYMENT_DISPLAYNAME %>">
<INPUT TYPE="HIDDEN" NAME="PAYMENT_CARDNUMBER" VALUE="<% =PAYMENT_CARDNUMBER %>">
<INPUT TYPE="HIDDEN" NAME="PAYMENT_EXMONTH" VALUE="<% =PAYMENT_EXMONTH %>">
<INPUT TYPE="HIDDEN" NAME="PAYMENT_EXYEAR" VALUE="<% =PAYMENT_EXYEAR %>">
<INPUT TYPE="HIDDEN" NAME="PAYMENT_CARDHOLDER" VALUE="<% =PAYMENT_CARDHOLDER %>">
<INPUT TYPE="HIDDEN" NAME="SIGNED_PID" VALUE="<% =SIGNED_PID %>">
<INPUT TYPE="HIDDEN" NAME="EMAILNAME" VALUE="<% =EMAILNAME %>">
<INPUT TYPE="HIDDEN" NAME="EMAILPASSWORD" VALUE="<% =EMAILPASSWORD %>">
<INPUT TYPE="HIDDEN" NAME="POPSELECTION" VALUE="<% =POPSELECTION %>">
</FORM></BODY>
</HTML>