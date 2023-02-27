import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime


def get_memory_info():
    with open('results.txt', 'r') as f:
        content = f.read()

    content = content[content.find(':')+1:]
    total = int(content[:content.find('k')]) / 1024
    content = content[content.find('\n')+2:]

    time_list = list()
    available_list = list()
    free_list = list()
    occupied_list = list()
    splitted = content.split()

    for i, item in enumerate(splitted):
        if item.lower() == "time":
            y, m, d = str(datetime.now()).split(' ', 2)[0].split('-', 3)
            time_list.append(datetime.strptime(f"{m}/{d}/{y[2:]} " + splitted[i + 1], '%m/%d/%y %H:%M:%S'))
        if item.lower() == "available:":
            available_list.append(int(splitted[i + 1]) / 1024)
        if item.lower() == "free:":
            free_list.append(int(splitted[i + 1]) / 1024)
        if item.lower() == "occupied:":
            occupied_list.append(int(splitted[i + 1]) / 1024)
    
    #print(available_list)

    return total, time_list, available_list, free_list, occupied_list


def main():
    total_memory, time_list, available_list, free_memory, occupied_list = get_memory_info()
    fig, ax = plt.subplots()

    time_points = np.array(time_list)
    available_points = np.array(available_list)
    free_points = np.array(free_memory)
    occupied_points = np.array(occupied_list)

    plt.title('Memory usage statistics')
    plt.xlabel('Time')
    plt.ylabel('Memory usage (MB)')

    ax.plot(time_points, available_points, label='Available memory (MB)')
    ax.plot(time_points, free_points, label='Free memory (MB)', linestyle='dashed')
    ax.plot(time_points, occupied_points, label='Occupied memory (MB)', linestyle=':')
    ax.legend()

    #plt.ylim([min(occupied_points), total_memory])
    plt.show()


if __name__ == '__main__':
    main()
