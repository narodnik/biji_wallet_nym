import asyncio
import json
import nym_proxy

def fetch_history(addresses):
    for address in addresses:
        print(address)

async def send_response(nym, history, request_id, client_id):
    data = json.dumps({
        "id": None,
        "data": "hello"
    }).encode()
    client = await nym_proxy.find_client(self.nym, client_id)
    await self.nym.send(data, client)

async def run_server():
    nym = nym_proxy.NymProxy(port=19001)
    await nym.start()

    my_details = await nym.details()
    print(my_details)
    print("Server started.")
    print()

    while True:
        messages = []
        while not messages:
            messages = await nym.fetch()
            await asyncio.sleep(0.1)

        print("Messages:", messages)

        for message in messages:
            if message == b"hello":
                print("Received 'hello'")
                continue

            request = json.loads(message.decode())
            if request["command"] == "fetch_history":
                history = fetch_history(request["data"])
                await send_response(nym, history,
                                    request["id"], request["client"])

asyncio.run(run_server())

