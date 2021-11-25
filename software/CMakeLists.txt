add_executable(lcd_1602_i2c
        lcd_1602_i2c.c
        )

# Pull in our (to be renamed) simple get you started dependencies
target_link_libraries(lcd_1602_i2c pico_stdlib hardware_i2c hardware_adc hardware_pwm)

# create map/bin/hex file etc.
pico_add_extra_outputs(lcd_1602_i2c)

# add url via pico_set_program_url
example_auto_set_url(lcd_1602_i2c)
