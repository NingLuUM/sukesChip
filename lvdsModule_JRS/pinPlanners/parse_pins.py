import csv
import numpy as np
# pin name 			= col0
# pin direction 	= col1
# pin planne loc	= col2
# pin_bank 			= col3
lvds,txrx=[],[]

#~ with open('lvds_pin_planner.csv') as csv_file:
#~ with open('lvds_pins_trident.csv') as csv_file:
with open('lvds_pins_minion.csv') as csv_file:
	csv_reader = csv.reader(csv_file, delimiter=',')
	
	for row in csv_reader:
		if row[3] != '':
			lvds.append([row[0],row[2],None,row[3]])


with open('txrx_pin_planner.csv') as csv_file:
	csv_reader = csv.reader(csv_file, delimiter=',')
	
	for row in csv_reader:
		for n in range(0,len(lvds)):
			if row[2] == lvds[n][1]:
				lvds[n][2] = row[0]
				break
	
for row in lvds:
	if row[3] == '4A':			
		print '%8s,' % row[1],
		print '%15s, ' % row[0],
		print '%15s' % row[2],
		print '%15s' % row[3]
print '\n'
for row in lvds:
	if row[3] != '4A':			
		print '%8s,' % row[1],
		print '%15s, ' % row[0],
		print '%15s' % row[2],
		print '%15s' % row[3]
				

	

            #~ print 'Column names are', {", ".join(row)}
            #~ line_count += 1
        #~ else:
            #~ print '\t',{row[0]},' works in the', {row[1]},'department,  and was born in',{row[2]}
            #~ line_count += 1
    #~ print 'Processed',{line_count}, 'lines.'
