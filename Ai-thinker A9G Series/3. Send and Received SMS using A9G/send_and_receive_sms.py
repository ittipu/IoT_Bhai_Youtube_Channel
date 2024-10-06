import cellular
import time

WAIT_FOR_SIM_INIT_TIMEOUT = 20 # seconds
WAIT_FOR_NETWORK_TIMEOUT = 30  # seconds

recipient_number = ""
message_content = ""
admin_number = "+8801715497977"
message_to_send = "A9G ready"


def sms_handler(evt):
    if evt == cellular.SMS_SENT:
        print("Startup SMS sent Successfully")

def interpret_network_status(status_code):
    if status_code == 0:
        return "Not registered, not searching"
    elif status_code == 1:
        return "Registered, home network"
    elif status_code == 2:
        return "Not registered, searching for network"
    elif status_code == 3:
        return "Registration denied"
    elif status_code == 4:
        return "Unknown status"
    elif status_code == 5:
        return "Registered, roaming"
    else:
        return "Invalid status code"

def main():
    print("Getting Sim Info: ")
    print("Wait {} seconds.........".format( WAIT_FOR_SIM_INIT_TIMEOUT))
    time.sleep(WAIT_FOR_SIM_INIT_TIMEOUT)
    
    print("Is Sim inserted: ", cellular.is_sim_present())
    print("ICCID: ", cellular.get_iccid())
    print("IMSI: ", cellular.get_imsi())
    print()
    
    print("Searching For Network: ")
    print("Wait {} seconds..........".format(WAIT_FOR_NETWORK_TIMEOUT))
    time.sleep(WAIT_FOR_NETWORK_TIMEOUT)
    print("Is Network Registered: ", cellular.is_network_registered())
    network_status_code = cellular.get_network_status()
    network_status_message = interpret_network_status(network_status_code)
    print("Network Status:", network_status_message)
    
    operator_info = cellular.register()
    print("Operator Name: {}".format(operator_info[1]))
    signal_quality = cellular.get_signal_quality()
    
    print("Signal Quality: ",signal_quality[0] )
    cellular.on_sms(sms_handler)
    print("Sending startup message-")
    cellular.SMS(admin_number, message_to_send).send()
    

    print("Checking for new SMS messages ...")
    while True:
        messages_list = cellular.SMS.list()
        messages_list = [msg for msg in messages_list if msg is not None]

        # Check if there are any messages in the list
        for msg in messages_list:
            if msg is not None:
                try:
                    recipient_number = msg.phone_number  # Extract phone number
                    message_content = msg.message  # Extract message content
                    print("\nNew message received!")
                    print("From: {}".format(recipient_number))
                    print("Message: {}".format(message_content))
                    
                    # Optionally delete the message after reading
                    msg.withdraw()
                    
                except AttributeError as e:
                    print("Error accessing message attributes: {}".format(e))

main()
