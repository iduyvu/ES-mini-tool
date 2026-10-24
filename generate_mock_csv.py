import csv
import random

def generate_csv(filename, num_rows):
    header = ["No.", "Description", "MAIN_WH", "ZCL", "CLCUS", "WRR", "SITE_A", "SITE_B"]

    with open(filename, mode='w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(header)

        for i in range(1, num_rows + 1):
            part_no = f"PART-{1000 + i}"
            description = f"Sample Description for Part {i}, Rev A"
            if i % 10 == 0:
                part_no = f"ZCL-{1000 + i}" # Generate some ZCL items

            main_wh = random.randint(0, 100)
            zcl = random.randint(0, 50) if "ZCL" in part_no else 0
            clcus = random.randint(0, 20)
            wrr = random.randint(0, 10)
            site_a = random.randint(0, 5)
            site_b = random.randint(0, 5)

            writer.writerow([part_no, f'"{description}"', main_wh, zcl, clcus, wrr, site_a, site_b])

if __name__ == "__main__":
    generate_csv("bin/nav_inventory.csv", 500)
    print("Mock CSV generated successfully at bin/nav_inventory.csv")
