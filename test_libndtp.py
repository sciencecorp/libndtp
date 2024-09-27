# test_libndtp.py
import libndtp


def main():
    # Create and pack an ExampleMessage
    packed = libndtp.create_message(123, "Test Name")
    print(f"Packed Message: {packed}")

    # Unpack the message
    unpacked = libndtp.parse_message(packed)
    print(f"Unpacked Message: ID={unpacked.id}, Name={unpacked.name}")


if __name__ == "__main__":
    main()
