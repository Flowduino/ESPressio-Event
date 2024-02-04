# ESPressio Event
Event-Driven Development (Even Pattern) Components of the Flowduino ESPressio Development Platform

Provides a foundation for designing, structuring, and implementing your embedded programs using Event Pattern (Event-Driven Development or "EDD").

## Latest Stable Version
There is currently no stable released version.

## ESPressio Development Platform
The **ESPressio** Development Platform is a collection of discrete (sometimes intra-connected) Component Libraries developed with a particular development ethos in mind.

The key objectives of the ESPressio Development Platform are:
- **Light-weight** - The Components should always strive to optimize memory consumption and operational overhead as much as possible, but not to the detriment of...
- **Ease of Use** - Many of our components serve as Developer-Friendly Abstractions of existing procedural code libraries.
- **Object-Oriented** - A `type` for everything, and everything in a `type`!
- **SOLID**:
- -  > **S**ingle Responsibility Principle (SRP)
    Break your code into smaller, focused components.
- - > **O**pen/Closed Principle (OCP)
    Be open for extension but closed for modification.
- - > **L**iskov Substitution Principle (LSP)
    Be substitutable for the base type without altering correctness.
- - > **I**nterface Segregation Principle (ISP)
    Break interfaces into specific, client-focused ones.
- - > **D**ependency Inversion Principle (DIP)
    Be dependent on abstractions, not concretions.

To the maximum extent possible within the limitations/restrictons/constraints of the C++ langauge, the Arduino platform, and Microcontroller Programming itself, all Component Libraries of the **ESPressio** Development Platform must strive to honour the **SOLID** principles.

## License
ESPressio (and its component libraries, including this one) are subject to the *Apache License 2.0*
Please see the [![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE) accompanying this library for full details.

## Namespace
Every type/variable/constant/etc. related to *ESPressio* Event are located within the `Event` submaspace of the `ESPressio` parent namespace.

## Platformio.ini
You can quickly and easily add this library to your project in PlatformIO by simply including the following in your `platformio.ini` file:

```ini
lib_deps = 
    https://github.com/Flowduino/ESPressio-Base.git
    https://github.com/Flowduino/ESPressio-Threads.git
    https://github.com/Flowduino/ESPressio-Event.git
```

Please note that this will use the very latest commits pushed into the repository, so volatility is possible.
This will of course be resolved when the first release version is tagged and published.
This section of the README will be updated concurrently with each release.
