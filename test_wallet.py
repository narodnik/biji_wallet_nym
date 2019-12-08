import asyncio
import json
import nym_proxy
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

class Proxy:

    def __init__(self):
        self.nym = nym_proxy.NymProxy()

    async def start(self):
        await self.nym.start()

        self.my_details = await self.nym.details()
        print(self.my_details)
        print("Client started.")
        print()

        server_id = "F9xzbjnMQVN4ZidcqN2ip9kVnI9wbS39aVayZGiMihY="
        self.server = await nym_proxy.find_client(self.nym, server_id)
        await self.nym.send(b"hello", self.server)

    async def get_history(self, keys):
        addresses = [wallet.key_to_address(key) for key in keys]
        data = json.dumps({
            "command": "fetch_history",
            "id": None,     # Field currently unused
            "client": self.my_details.Id,
            "data": addresses
        }).encode()

        await self.nym.send(data, self.server)

        # Wait for messages using long poll loop
        messages = []
        while not messages:
            messages = await self.nym.fetch()
            await asyncio.sleep(0.1)

        print("Messages:", messages)

async def run_wallet():
    proxy = Proxy()
    await proxy.start()

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
            histories = await proxy.get_history(keys)
            #for address, history in histories.items():
            if False:
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

asyncio.run(run_wallet())

