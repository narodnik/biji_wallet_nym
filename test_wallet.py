import wallet

def get_history(keys):
    addresses = [wallet.key_to_address(key) for key in keys]
    histories = wallet.fetch_history(addresses)
    return histories

def prompt_destinations():
    destinations = []

    while True:
        address = input("Address: ")
        amount = wallet.decode_base10(input("Amount: "))
        if amount is None:
            print("Bad amount... skipping")
            continue
        destinations.append((address, amount))

        is_continue = input("Add another output? [y/N] ")
        if is_continue != "y" and is_continue != "Y":
            break

    return destinations

keys = [
    "cVks5KCc8BBVhWnTJSLjr5odLbNrWK9UY4KprciJJ9dqiDBenhzr"
]

stopped = False
while not stopped:
    print("MAIN MENU")
    print("1. New key")
    print("2. Receive addresses")
    print("3. Show history and balance")
    print("4. Send funds")
    print("5. Exit")

    choice = int(input("> "))
    if choice == 1:
        keys.append(wallet.new_key())
        for key in keys:
            print(key)
    elif choice == 2:
        for key in keys:
            print(wallet.key_to_address(key))
    elif choice == 3:
        total_value = 0
        histories = get_history(keys)
        for address, history in histories.items():
            print(address)
            for row in history:
                print("  output hash=%s index=%s height=%s" %
                      (row.output.hash.hex(), row.output.index,
                       row.output.height))
                print("  +%s" % wallet.encode_base10(row.value))
                #print("row spend", row.spend, type(row.spend))
                #print(bool(row.spend))
                #print(row.spend.hash)
                #print(row.spend.index)
                #print(row.spend.height)
                if row.spend is not None:
                    print("    spend hash=%s index=%s height=%s" %
                          (row.spend.hash.hex(), row.spend.index,
                           row.spend.height))
                    print("    -%s" % wallet.encode_base10(row.value))
                else:
                    total_value += row.value
        print("Balance:", wallet.encode_base10(total_value))
    elif choice == 4:
        destinations = prompt_destinations()
        histories = get_history(keys)
        total_value = sum(value for (address, value) in destinations)
        unspent = wallet.select_outputs(keys, total_value, histories)
        tx_data = wallet.build_transaction(destinations, keys, unspent)
        print(unspent)
        print("tx_data =", tx_data.hex())
    elif choice == 5:
        stopped = True

