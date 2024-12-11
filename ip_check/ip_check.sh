#!/bin/bash

# Load environment variables
source .env 

# Set file to store the previous IP address
IP_FILE="ip_address.txt"  # Path to save the last known IP

# Get the current private IP address (eth0 or wlan0 depending on your setup)
CURRENT_IP=$(hostname -I | awk '{print $1}')

# Check if IP file exists, and if not, create it
if [ ! -f "$IP_FILE" ]; then
    echo "$CURRENT_IP" > "$IP_FILE"
    exit 0  # First run, exit after storing the initial IP
fi

# Read the last known IP from the file
LAST_IP=$(cat "$IP_FILE")

# If the IP has changed, update the file and send an email
if [ "$CURRENT_IP" != "$LAST_IP" ]; then
    # Update the stored IP address
    echo "$CURRENT_IP" > "$IP_FILE"

    # Compose the email
    SUBJECT="IP Address Change Notification"
    BODY="The private IP address of the device has changed. New IP: $CURRENT_IP"

    # Send the email using curl
    echo -e "Subject: $SUBJECT\n\n$BODY" | curl --ssl-reqd \
        --url "smtps://smtp.gmail.com:465" \
        --mail-from "$EMAIL_ADDRESS" \
        --mail-rcpt "$RECIPIENT" \
        --user "$EMAIL_ADDRESS:$SMTP_PASSWORD" \
        --upload-file -
fi