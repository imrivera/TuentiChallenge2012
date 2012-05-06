#!/usr/bin/env python
# -*- coding: utf-8 -*-

# *** Challenge 1: The cell phone keypad ***
#
# This version calculates the minimum possible
# time. But it seems that they weren't asking 
# for this answer.
#
# The user can reduce its time planning ahead
# the shift lock press


import sys

press_button_time = 100
wait_button_time = 500

active_keys = [ [1, 1, 1],
                [1, 1, 1],
                [1, 1, 1],
                [0, 1, 1] ]

letters = {    ' 1': (0, 0), 'ABC2': (0, 1),  'DEF3': (0, 2),
             'GHI4': (1, 0), 'JKL5': (1, 1),  'MNO6': (1, 2),
            'PQRS7': (2, 0), 'TUV8': (2, 1), 'WXYZ9': (2, 2),
                                '0': (3, 1) }

letters_uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
letters_lowercase = "abcdefghijklmnopqrstuvwxyz"

caps_lock_key = (3, 2)

move_costs = [ ( 300, -1,  0 ),
               ( 300,  1,  0 ),
               ( 200,  0, -1 ),
               ( 200,  0,  1 ),
               ( 350,  1,  1 ),
               ( 350, -1,  1 ),
               ( 350,  1, -1 ),
               ( 350, -1, -1 ) ]

infinity = sys.maxint


def get_neighbours(pos):
    y = pos[0]
    x = pos[1]
    neighbours = []
    for m in move_costs:
        new_y = y + m[1]
        new_x = x + m[2]
        if new_x >= 0 and new_x < 3 and new_y >= 0 and new_y < 4 and active_keys[new_y][new_x] == 1:
            neighbours.append((m[0], (new_y, new_x)))

    return neighbours

    

def shortest_path(orig):
    vertex_list = set([(y,x) for x in range(3) for y in range(4)])
    dist = dict([(pos, infinity) for pos in vertex_list])

    if active_keys[orig[0]][orig[1]] == 0:
        return dist

    dist[orig] = 0

    while len(vertex_list) > 0:
        min_dist = infinity
        for vertex in vertex_list:
            if dist[vertex] <= min_dist:
                u = vertex
                min_dist = dist[vertex]

        if dist[u] == infinity:
            break

        vertex_list.remove(u)
        neighbours = get_neighbours(u)
        for cost, v in neighbours:
            alt = dist[u] + cost
            if alt < dist[v]:
                dist[v] = alt
    
    return dist
                

costs = dict([((y,x),shortest_path((y,x))) for x in range(3) for y in range(4)])
initial_caps_lock_enabled = False
initial_key = (3, 1)

def is_caps_lock_change_needed(text, caps_lock_enabled):
    for i in range(len(text)):
        c = text[i]
        if c.upper() in letters_uppercase:
            if c in letters_uppercase and not caps_lock_enabled:
                return (True, i)
            elif c in letters_lowercase and caps_lock_enabled:
                return (True, i)
            else:
                return (False, i)
        
    return (False, i)


def find_key(c):
    upper_c = c.upper()
    for key in letters:
        if upper_c in key:
            return key, letters[key]



def calculate_minimal_time(first_time, text, current_key, caps_lock_enabled):
    if len(text) == 0:
        return 0

    minimal_time = 0
    upper_c = text[0].upper()

    caps_lock_should_change, when_change_caps_lock = is_caps_lock_change_needed(text, caps_lock_enabled)
    if caps_lock_should_change == True:
        time_changing = costs[current_key][caps_lock_key] + press_button_time
        time_changing += calculate_minimal_time(False, text, caps_lock_key, not caps_lock_enabled)

        minimal_time = time_changing
        
        if when_change_caps_lock > 0:
            letters_in_button, dest_key = find_key(text[0])            
            time_without_caps_lock = costs[current_key][dest_key] + press_button_time
            time_without_caps_lock += (letters_in_button.find(upper_c) + 1) * 100
            if current_key == dest_key and not first_time:
                time_without_caps_lock += 500
            time_without_caps_lock += calculate_minimal_time(False, text[1:], dest_key, caps_lock_enabled)
            minimal_time = min(time_without_caps_lock, time_changing)           
    else:
        letters_in_button, dest_key = find_key(text[0])
        minimal_time = costs[current_key][dest_key]
        minimal_time += (letters_in_button.find(upper_c) + 1) * 100
        if current_key == dest_key and not first_time:
            minimal_time += 500
        minimal_time += calculate_minimal_time(False, text[1:], dest_key, caps_lock_enabled)
        
    return minimal_time
    

count = int(sys.stdin.readline())

for i in range(count):
    text = sys.stdin.readline().rstrip()

    print calculate_minimal_time(True, text, initial_key, initial_caps_lock_enabled)



    
    
    
