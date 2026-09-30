# Model-Rocket-Flight-Computer-
A model rocket flight computer for UKROC. Requires an Adafruit Feather nRF52840 Sense, Adafruit Ultimate GPS featherwing and Adafruit Adalogger FeatherWing

During flight, the onboard flight computer will record data using sensors such as
pressure, acceleration, direction, and GPS. It will aim to measure at a sample
rate of ≥500–1,000 Hz to ensure accuracy and, for timing, will use the RTC to
timestamp data in addition to GPS time.
For storage, an SD card will constantly log and write data to memory, whereas
for power, a 500 mAh LiPo battery with a runtime of 2–3 hours will be used,
providing safe charging and regulation for 3.3 V logic. This should ensure that
the flight computer has sufficient battery life and storage capacity for up to 2
hours of operation.
Finally, the flight computer is not a mission-critical component, meaning that in
the event of a failure, the rocket will continue to function normally.
