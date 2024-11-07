from neopixel import Neopixel
import utime
import random
import uasyncio as asyncio
import panel_maps


class Row:
    def __init__(self, min_pixel: int, max_pixel: int, leftToRightFunction: int, pixel_colour_map: dict):
        self.min_pixel = min_pixel
        self.max_pixel = max_pixel
        self.leftToRightFunction = leftToRightFunction
        self.pixel_colour_map = pixel_colour_map

    def get_all_values(self):
        return f"""min_pixel: {self.min_pixel}, max_pixel: {self.max_pixel}, function:{self.leftToRightFunction}, pixelMap: {self.pixel_colour_map}"""

    # Shift all pixels right providing at least one if within the range of visible pixels
    def shift_pixels_right(self):
        new_pixel_map = {}

        all_pixels_out_of_bounds = True
        if self.leftToRightFunction == 1:
            for pixel in self.pixel_colour_map.keys():
                if pixel < self.max_pixel:
                    all_pixels_out_of_bounds = False
                    break
        else:
            for pixel in self.pixel_colour_map.keys():
                if pixel > self.min_pixel:
                    all_pixels_out_of_bounds = False
                    break

        if all_pixels_out_of_bounds:
            return

        for pixel, colour in self.pixel_colour_map.items():
            new_pixel = pixel + 1 * self.leftToRightFunction
            new_pixel_map[new_pixel] = colour
        self.pixel_colour_map = new_pixel_map

    # Shift all pixels left providing at least one if within the range of visible pixels
    def shift_pixels_left(self):
        new_pixel_map = {}

        all_pixels_out_of_bounds = True
        if self.leftToRightFunction == 1:
            for pixel in self.pixel_colour_map.keys():
                if pixel > self.min_pixel:
                    all_pixels_out_of_bounds = False
                    break
        else:
            for pixel in self.pixel_colour_map.keys():
                if pixel < self.max_pixel:
                    all_pixels_out_of_bounds = False
                    break

        if all_pixels_out_of_bounds:
            return

        for pixel, colour in self.pixel_colour_map.items():
            new_pixel = pixel - 1 * self.leftToRightFunction
            new_pixel_map[new_pixel] = colour
        self.pixel_colour_map = new_pixel_map

    def is_any_pixels_in_min_max_range(self):
        for pixel in self.pixel_colour_map.keys():
            if self.min_pixel < pixel < self.max_pixel:
                return True
        return False

    def set_pixel_map(self, pixel_map):
        self.pixel_colour_map = pixel_map

    def shift_pixels_down(self):
        if not self.is_any_pixels_in_min_max_range():
            print("none showing")
            return

        # get new pixel numbers
        new_pixel_map = {}
        for pixel_number, pixel_colour in self.pixel_colour_map.items():
            new_pixel_number = pixel_number + ((self.max_pixel - pixel_number) * 2) + 1
            new_pixel_map[new_pixel_number] = pixel_colour

        # update the min and max pixel
        number_of_pixels_per_row = self.max_pixel - self.min_pixel
        self.min_pixel = self.max_pixel + 1
        self.max_pixel = self.max_pixel + number_of_pixels_per_row + 1

        self.set_pixel_map(new_pixel_map)
        self.leftToRightFunction = self.leftToRightFunction * -1

    def shift_pixels_up(self):
        if not self.is_any_pixels_in_min_max_range():
            print("none showing")
            return

        # get new pixel numbers
        new_pixel_map = {}
        for pixel_number, pixel_colour in self.pixel_colour_map.items():
            new_pixel_number = pixel_number - ((pixel_number - self.min_pixel) * 2) - 1
            new_pixel_map[new_pixel_number] = pixel_colour

        # update the min and max pixel
        number_of_pixels_per_row = self.max_pixel - self.min_pixel
        self.max_pixel = self.min_pixel - 1
        self.min_pixel = self.min_pixel - number_of_pixels_per_row - 1

        self.set_pixel_map(new_pixel_map)
        self.leftToRightFunction = self.leftToRightFunction * -1


class Panel:
    def __init__(self, min_pixel, max_pixel, number_of_rows):
        self.min_pixel = min_pixel
        self.max_pixel = max_pixel
        self.number_of_rows = number_of_rows
        rows = {}
        self.pixels_per_row = (max_pixel - min_pixel) / number_of_rows

        for row_number in range(number_of_rows):
            row_min = min_pixel + row_number * self.pixels_per_row
            row_max = row_min + self.pixels_per_row - 1

            left_to_right_function = 1
            if row_number % 2 == 1:
                left_to_right_function = -1

            rows[row_number + 1] = (Row(int(row_min), int(row_max), int(left_to_right_function), {}))
        self.rows = rows

    def printAllRowValues(self):
        for row_number, row in self.rows.items():
            print(f"{row_number}, {row.get_all_values()}")

    def setRowPixelMap(self, row_number, pixel_map):
        self.rows[row_number].set_pixel_map(pixel_map)

    def setAllRowPixelMap(self, row_number_to_pixel_map):
        for row_number, pixel_map in row_number_to_pixel_map.items():
            self.rows[row_number].set_pixel_map(pixel_map)
        for row_number in range(self.number_of_rows):
            row_min = self.min_pixel + row_number * self.pixels_per_row
            row_max = row_min + self.pixels_per_row - 1
            self.rows[row_number + 1].min_pixel = int(row_min)
            self.rows[row_number + 1].max_pixel = int(row_max)
            left_to_right_function = 1
            if row_number % 2 == 1:
                left_to_right_function = -1
            self.rows[row_number + 1].leftToRightFunction = int(left_to_right_function)

    def clearAllRows(self):
        for row in self.rows.values():
            row.set_pixel_map({})

    def shiftAllRowsLeft(self):
        for row in self.rows.values():
            row.shift_pixels_left()

    def shiftAllRowsRight(self):
        for row in self.rows.values():
            row.shift_pixels_right()

    def shiftAllRowsDown(self):
        if not self.isAnyPixelInBoundsOnAllRows():
            return

        for row_number, row in self.rows.items():
            row.shift_pixels_down()

    def shiftAllRowsUp(self):
        if not self.isAnyPixelInBoundsOnAllRows():
            return

        for row_number, row in self.rows.items():
            row.shift_pixels_up()

    def isAnyPixelInBoundsOnAllRows(self):
        for row in self.rows.values():
            if row.is_any_pixels_in_min_max_range():
                return True
        return False


class LedHandler:
    numpix = 150
    pin_number = 0
    strip = Neopixel(numpix, 0, pin_number, "RGB")
    strip.brightness(42)

    red = (255, 0, 0)
    green = (0, 255, 0)
    blue = (0, 0, 255)
    orange = (50, 255, 0)
    yellow = (100, 255, 0)
    indigo = (0, 100, 90)
    violet = (0, 200, 100)

    frame_update_delay = 0.01

    blank = (0, 0, 0)

    current_task = "STOP"
    message = "AB"

    pixels_to_show = []
    start_position = 0
    pointer = 0

    stored_panel_maps = [
        panel_maps.fuck_off_orange,
        panel_maps.plus_map_1,
        panel_maps.wave_map_1,
        panel_maps.sea_bass_map,
        panel_maps.sea_bass_map_2,
        panel_maps.lines_map_1
    ]

    current_stored_panel_map_index = 0

    panel1 = Panel(0, 150, 5)
    panel1.setAllRowPixelMap(
        stored_panel_maps[current_stored_panel_map_index]
    )

    def __init__(self):
        print("handler")

    def set_task(self, task):
        self.current_task = task

    async def run(self):
        while True:
            if self.current_task == "BOUNCE":
                self.horizontal_bounce()
            elif self.current_task == "ALTERNATE_BOUNCE":
                self.alternate_horizontal_bounce()
            elif self.current_task == "STATIC_IMAGE":
                self.static_image(2)
            elif self.current_task == "NEXT_IMAGE":
                print("123")
                self.next_map()
            elif self.current_task == "FALLING":
                self.falling()
            elif self.current_task == "VERTICAL_BOUNCE":
                self.vertical_bounce()
            else:
                self.stop()
            await asyncio.sleep(self.frame_update_delay)

    def set_frame_update_delay(self, speed_percentage):
        if 90 < speed_percentage < 101:
            self.frame_update_delay = 0.01
        elif 80 < speed_percentage < 91:
            self.frame_update_delay = 0.02
        elif 70 < speed_percentage < 81:
            self.frame_update_delay = 0.04
        elif 60 < speed_percentage < 71:
            self.frame_update_delay = 0.05
        elif 50 < speed_percentage < 61:
            self.frame_update_delay = 0.06
        elif 40 < speed_percentage < 51:
            self.frame_update_delay = 0.07
        elif 30 < speed_percentage < 41:
            self.frame_update_delay = 0.08
        elif 20 < speed_percentage < 31:
            self.frame_update_delay = 0.09
        elif 10 < speed_percentage < 21:
            self.frame_update_delay = 0.1
        elif -1 < speed_percentage < 11:
            self.frame_update_delay = 0.2
    def stop(self):
        self.strip.fill(self.blank)
        self.strip.show()

    def falling(self):
        # print(f"test: {self.stored_panel_maps[self.current_stored_panel_map_index]}")
        self.panel1.setAllRowPixelMap(self.stored_panel_maps[self.current_stored_panel_map_index])
        self.static_image(self.frame_update_delay)

        self.strip.fill(self.blank)
        self.strip.show()
        # self.panel1.printAllRowValues()
        for i in range(self.panel1.number_of_rows):

            self.panel1.shiftAllRowsDown()
            for row in self.panel1.rows.values():
                for pixel, colour in row.pixel_colour_map.items():
                    if row.min_pixel <= pixel <= row.max_pixel and pixel < self.panel1.max_pixel:
                        self.strip.set_pixel(pixel, colour)
            self.strip.show()

            utime.sleep(self.frame_update_delay)

            self.strip.fill(self.blank)
            self.strip.show()

    def vertical_bounce(self):
        self.strip.fill(self.blank)
        self.strip.show()
        # self.panel1.printAllRowValues()
        for i in range(self.panel1.number_of_rows * 3):

            should_shift_down_more = False
            for row in self.panel1.rows.values():
                if row.min_pixel < self.panel1.max_pixel:
                    should_shift_down_more = True
                    break

            if not should_shift_down_more:
                break

            self.panel1.shiftAllRowsDown()
            for row in self.panel1.rows.values():
                for pixel, colour in row.pixel_colour_map.items():
                    if row.min_pixel <= pixel <= row.max_pixel and self.panel1.min_pixel <= pixel < self.panel1.max_pixel:
                        self.strip.set_pixel(pixel, colour)
            self.strip.show()

            utime.sleep(self.frame_update_delay)

            self.strip.fill(self.blank)
            self.strip.show()

        for i in range(self.panel1.number_of_rows * 3):

            should_shift_up_more = False
            for row in self.panel1.rows.values():
                if row.max_pixel > self.panel1.min_pixel:
                    should_shift_up_more = True
                    break

            if not should_shift_up_more:
                break

            self.panel1.shiftAllRowsUp()
            for row in self.panel1.rows.values():
                for pixel, colour in row.pixel_colour_map.items():
                    if row.min_pixel <= pixel <= row.max_pixel and self.panel1.min_pixel <= pixel < self.panel1.max_pixel:
                        self.strip.set_pixel(pixel, colour)
            self.strip.show()

            utime.sleep(self.frame_update_delay)

            self.strip.fill(self.blank)
            self.strip.show()

    def next_map(self):
        print("yo")
        if (len(self.stored_panel_maps) - 1 > self.current_stored_panel_map_index):
            self.current_stored_panel_map_index = self.current_stored_panel_map_index + 1
            print("here")
        else:
            print("else")
            self.current_stored_panel_map_index = 0
        self.static_image(2)
        self.set_task("STATIC_IMAGE")

    def static_image(self, delay):
        self.stop()
        self.panel1.setAllRowPixelMap(self.stored_panel_maps[self.current_stored_panel_map_index])
        for row in self.panel1.rows.values():
            for pixel, colour in row.pixel_colour_map.items():
                if row.min_pixel <= pixel <= row.max_pixel:
                    self.strip.set_pixel(pixel, colour)
        self.strip.show()
        utime.sleep(delay)

    def horizontal_bounce(self):
        self.stop()
        self.static_image(0.1)

        # The number of pixels shifts to be performed = number of pixels per row + num of pixels to be displayed
        amount_to_move = 0
        for row in self.panel1.rows.values():
            number_of_pixels_to_be_displayed = len(row.pixel_colour_map)
            if amount_to_move < number_of_pixels_to_be_displayed:
                amount_to_move = number_of_pixels_to_be_displayed
        amount_to_move += self.panel1.pixels_per_row

        for i in range(amount_to_move + 30):
            self.panel1.shiftAllRowsRight()

            for row in self.panel1.rows.values():
                for pixel, colour in row.pixel_colour_map.items():
                    if row.min_pixel <= pixel <= row.max_pixel:
                        self.strip.set_pixel(pixel, colour)
            self.strip.show()

            utime.sleep(self.frame_update_delay)

            self.strip.fill(self.blank)
            self.strip.show()

        for i in range(amount_to_move + 30):
            self.panel1.shiftAllRowsLeft()

            for row in self.panel1.rows.values():
                for pixel, colour in row.pixel_colour_map.items():
                    if row.min_pixel <= pixel <= row.max_pixel:
                        self.strip.set_pixel(pixel, colour)
            self.strip.show()

            utime.sleep(self.frame_update_delay)

            self.strip.fill(self.blank)
            self.strip.show()

    def alternate_horizontal_bounce(self):
        self.stop()
        self.static_image(0.1)

        # The number of pixels shifts to be performed = number of pixels per row + num of pixels to be displayed
        amount_to_move = 0
        for row in self.panel1.rows.values():
            number_of_pixels_to_be_displayed = len(row.pixel_colour_map)
            if amount_to_move < number_of_pixels_to_be_displayed:
                amount_to_move = number_of_pixels_to_be_displayed
        amount_to_move += self.panel1.pixels_per_row

        for i in range(amount_to_move):

            for row in self.panel1.rows.values():
                if row.leftToRightFunction == 1:
                    row.shift_pixels_right()
                else:
                    row.shift_pixels_left()
                for pixel, colour in row.pixel_colour_map.items():
                    if row.min_pixel <= pixel <= row.max_pixel:
                        self.strip.set_pixel(pixel, colour)
            self.strip.show()

            utime.sleep(self.frame_update_delay)

            self.strip.fill(self.blank)
            self.strip.show()

        for i in range(amount_to_move):

            for row in self.panel1.rows.values():
                if row.leftToRightFunction == 1:
                    row.shift_pixels_left()
                else:
                    row.shift_pixels_right()
                for pixel, colour in row.pixel_colour_map.items():
                    if row.min_pixel <= pixel <= row.max_pixel:
                        self.strip.set_pixel(pixel, colour)
            self.strip.show()

            utime.sleep(self.frame_update_delay)

            self.strip.fill(self.blank)
            self.strip.show()


