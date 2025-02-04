The script included in this directory checks the Raspberry PI for changes in its IP address.
It checks the IP address against whatever is stored in ip_address.txt. If the IP changes, it replaces the stored address
    and sends an email to whatever is specified in the variable assignements at the top of the file. It is configured to 
    send an email from a Gmail account using the Gmal SMTP server. I currently have it set to send from my personal 
    account to my UVA account.

To get this to continually check for IP address changes, stage it in 'crontab' to be repeated every 10-30 minutes 
    (depending on your patience)
