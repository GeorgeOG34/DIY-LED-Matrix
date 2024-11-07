from machine import Pin
import utime
import uasyncio as asyncio
from led_handler import LedHandler

led_strip = LedHandler()


async def main():
    current_effect_index = 0
    effects = ["STATIC_IMAGE", "BOUNCE", "ALTERNATE_BOUNCE", "FALLING", "VERTICAL_BOUNCE"]

    asyncio.create_task(led_strip.run())
    while True:
        print(1)
        led_strip.set_task("NEXT_IMAGE")
        await asyncio.sleep(10)
        print(2)
        led_strip.set_task("BOUNCE")
        await asyncio.sleep(40)
        led_strip.set_task("ALTERNATE_BOUNCE")
        await asyncio.sleep(40)
        led_strip.set_task("STATIC_IMAGE")
        await asyncio.sleep(10)
        led_strip.set_task("VERTICAL_BOUNCE")
        await asyncio.sleep(40)


# Create an Event Loop
loop = asyncio.get_event_loop()
# Create a task to run the main function
loop.create_task(main())

try:
    # Run the event loop indefinitely
    loop.run_forever()
except Exception as e:
    print('Error occured: ', e)
except KeyboardInterrupt:
    print('Program Interrupted by the user')
