from setuptools import setup

setup(
    name = "driver.py",
    version = "0.0.1",
    py_modules = ["driver"],
    install_requires = [
        "Click",
        "pyserial",
        "keyboard"
    ],
    entry_points = """
        [console_scripts]
        driver=driver:driver
    """,
)

