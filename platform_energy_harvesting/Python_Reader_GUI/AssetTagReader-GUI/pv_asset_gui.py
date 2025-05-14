#!/usr/bin/env python3


import sys
from tkinter import scrolledtext
import tkinter as tk
from tkinter import ttk,Label,Canvas
from tkinter import PhotoImage
import random

#Switch to control dummy vs. real data
DUMMY = False
class PVAssetTagAppDummy(): 
    def __init__(self):
        self.AssetTagData = dict()
        self.log = "log"
    def start(self):
        pass
    def stop(self):
        pass
    def reset(self):
        pass

class CustomLabel(tk.Label):
    def __init__(self, master=None, **kwargs):
        kwargs['font'] = kwargs.get('font', ('Helvetica', 15))
        kwargs['anchor'] = kwargs.get('anchor', 'center')
        #kwargs['borderwidth'] = kwargs.get('borderwidth', 3)  
        #kwargs['relief'] = kwargs.get('relief', 'solid')
        super().__init__(master, **kwargs)

class CustomCell(tk.Label):
    def __init__(self, master=None, **kwargs):
        kwargs['font'] = kwargs.get('font', ('Helvetica', 10))
        kwargs['anchor'] = kwargs.get('anchor', 'center')
        #kwargs['height'] = kwargs.get('height', 15)
        #kwargs['borderwidth'] = kwargs.get('borderwidth', 3)  
        #kwargs['relief'] = kwargs.get('relief', 'solid')
        super().__init__(master, **kwargs)

class GUIApp:
    """
    A class representing the GUI application for PV Asset tag control.

    Attributes:
        pvassttagapp: The PV Asset tag control application.
        root: The root Tkinter window.
        gui_table: A dictionary to store GUI table data.
        refresh_data_interval: The interval for refreshing data in milliseconds.
        frame: The main frame of the GUI.
        frame_buttons: The frame for buttons.
        frame_canvas: The frame for the canvas and scrollbar.
        canvas: The canvas for displaying table data.
        scrollbar: The vertical scrollbar for the canvas.
        frame2: The frame inside the canvas.
        heading_label1-7: The labels for table headings.
        scrolled_text: The scrolled text widget for displaying logs.

    Methods:
        __init__(self, app): Initializes the GUIApp object.
        setup_ui(self): Sets up the user interface.
        start_app_callback(self): Callback function for the Start button.
        stop_app_callback(self): Callback function for the Stop button.
        reset_app_callback(self): Callback function for the Reset button.
        run(self): Runs the GUI application.
        refresh_data(self): Refreshes the data in the GUI.
        refresh_table(self): Refreshes the table data in the GUI.
    """

    def __init__(self,pvassttagapp,thresholds):
        if DUMMY:
            self.pvassttagapp = PVAssetTagAppDummy()
        else:
            self.pvassttagapp = pvassttagapp
            self.log = self.pvassttagapp.log
        self.battery_high_threshold = thresholds[0]
        self.battery_low_threshold = thresholds[1]
        self.temperature_high_threshold = thresholds[2]
        self.temperature_low_threshold = thresholds[3]
        self.root = tk.Tk()
        self.root.title("Energy Harvest Asset tag")
        self.root.iconphoto(True, PhotoImage(file="./images/silabs_logo.png"))
        self.root.sizefrom("user")
        self.refresh_switch = False
        self.gui_table=dict()
        self.setup_ui()
        
    def setup_ui(self):
        self.root.config(background="white")    
        self.root.grid_rowconfigure(0, weight=1)
        self.root.grid_columnconfigure(0, weight=1)
        #Set up main frame
        self.frame = tk.Frame(self.root)
        self.frame.config(background="white")
        self.frame.configure(width= self.root.winfo_width())
        self.frame.grid(row=0, column=0,sticky="nsew")
        self.frame.grid_rowconfigure(0, weight=1)
        self.frame.grid_columnconfigure(0, weight=1)
        self.frame.grid_rowconfigure(1, weight=1)
        # Set up image paths
        self.img_cold_notactive = PhotoImage(file="./images/cold_not_active.png")
        self.img_cold_active = PhotoImage(file="./images/cold_active.png")
        self.img_hot_notactive = PhotoImage(file="./images/hot_not_active.png")
        self.img_hot_active = PhotoImage(file="./images/hot_active.png")
        self.img_temp_low_power = PhotoImage(file="./images/temp_low_power.png")

        self.img_battery_100_percent = PhotoImage(file="./images/battery_100.png")
        self.img_battery_75_percent = PhotoImage(file="./images/battery_75.png")
        self.img_battery_50_percent = PhotoImage(file="./images/battery_50.png")
        self.img_battery_25_percent = PhotoImage(file="./images/battery_25.png")
        self.img_battery_10_percent = PhotoImage(file="./images/battery_10.png")
        self.img_battery_0_percent = PhotoImage(file="./images/battery_0.png")
        self.img_battery_low_power = PhotoImage(file="./images/battery_low_power.png")

        #set up a frame for butttons 
        self.frame_buttons = tk.Frame(self.frame, bg="white")
        self.frame_buttons.configure(width= self.frame.winfo_width())   
        self.frame_buttons.grid(row=0, column=0, columnspan=7, pady=10, sticky="ew")
        self.frame_buttons.grid_anchor("center")

        start_button = ttk.Button(self.frame_buttons,text="Start", command=self.start_app_callback)
        start_button.grid(row=0, column=0, padx=5, pady=5, sticky="ew")
        
        quit_button = ttk.Button(self.frame_buttons, text="Quit", command=self.stop_app_callback)
        quit_button.grid(row=0, column=1, padx=5, pady=5, sticky="ew")
        
        reset_button = ttk.Button(self.frame_buttons, text="Reset",command=self.reset_app_callback)
        reset_button.grid(row=0, column=2, padx=5, pady=5, sticky="ew")
        reset_button.grid_anchor("center")
        
        self.frame.grid_columnconfigure(0, weight=1)
        self.frame_buttons.grid_columnconfigure(0, weight=1)
        self.frame_buttons.grid_columnconfigure(1, weight=1)
        self.frame_buttons.grid_columnconfigure(2, weight=1)
        self.frame_buttons.grid_rowconfigure(0, weight=1)   
        
        self.frame2 = tk.Frame(self.frame, bg="white")
        self.frame2.configure(width= self.frame.winfo_width())
        self.frame2.grid(row=1, column=0, pady=10, sticky="ew", columnspan=10)

        # Create a row of label widgets
        self.heading_label1 = CustomLabel(self.frame2, background="white", text="Device ID")
        self.heading_label1.grid(row=0, column=0, sticky="ew")  
        self.heading_label2 = CustomLabel(self.frame2,background="white", text="Timestamp")
        self.heading_label2.grid(row=0, column=1, sticky="ew")
        self.heading_label3 = CustomLabel(self.frame2,background="white", text="MAC")
        self.heading_label3.grid(row=0, column=2, sticky="ew")
        self.heading_label4 = CustomLabel(self.frame2,background="white", text="Energy level")
        self.heading_label4.grid(row=0, column=3, sticky="ew")
        self.heading_label6 = CustomLabel(self.frame2,background="white", text="Temperature")
        self.heading_label6.grid(row=0, column=4, columnspan=2,sticky="ew")
        
        self.frame2.grid_columnconfigure((0,1,2,3,4), weight=1)
        self.frame2.grid_rowconfigure(0, weight=1)
        self.frame2.grid_rowconfigure(1, weight=1)
        self.frame2.grid_rowconfigure(2, weight=1)
        self.frame2.grid_rowconfigure(3, weight=1)
        self.frame2.grid_rowconfigure(4, weight=1)
        
        # Configure grid weights to make the UI responsive
        self.frame2.grid_columnconfigure(0, weight=1)
        self.frame2.grid_columnconfigure(1, weight=1)
        self.frame2.grid_columnconfigure(2, weight=1)
        self.frame2.grid_columnconfigure(3, weight=1)
        self.frame2.grid_columnconfigure(4, weight=1)
        self.frame2.grid_columnconfigure(5, weight=1)
        self.frame2.grid_columnconfigure(6, weight=1)
       
        # Create a scrolled text widget
        self.scrolled_text = scrolledtext.ScrolledText(self.frame, wrap=tk.WORD, width=60, height=10)
        self.scrolled_text.configure(bg="black", fg="yellow")
        self.scrolled_text.grid(row=3, column=0, columnspan=7, pady=10, sticky="nsew")
    
        # Configure grid weights to make the UI responsive
        self.frame.grid_columnconfigure(0, weight=1)
        self.frame.grid_columnconfigure(1, weight=1)
        self.frame.grid_columnconfigure(2, weight=1)
        self.frame.grid_columnconfigure(3, weight=1)
        self.frame.grid_columnconfigure(4, weight=1)
        self.frame.grid_columnconfigure(5, weight=1)
        self.frame.grid_columnconfigure(6, weight=1)
        self.frame.rowconfigure(3, weight=1)
    
    def start_app_callback(self):
        if DUMMY == False and self.refresh_switch == False:
            self.pvassttagapp.start()
        self.scrolled_text.insert(tk.END, f"Scanning started...\n")
        self.refresh_switch = True
        self.refresh_data_interval = 1000  # Refresh every 1000 ms (1 second      
        self.refresh_data()
    
    def stop_app_callback(self):
        if DUMMY == False:
            self.pvassttagapp.stop(self.root)
        self.refresh_switch = False
        self.scrolled_text.insert(tk.END, f"Scanning stopped...\n")
        

    def reset_app_callback(self):
        if DUMMY == False:
            self.pvassttagapp.reset()

    def run(self):
        self.root.mainloop()

    def refresh_data(self):
        if self.refresh_switch:
            if DUMMY == True:
                self.dummy_AssetTagData()
            self.display_debug()
            self.refresh_table()
            self.root.after(self.refresh_data_interval, self.refresh_data)  # Schedule next refresh
    
    def select_battery_images(self,entry):
        if entry["Vcap"] == None:
             return self.img_battery_low_power
        else:
            #sanity check
            if self.battery_high_threshold < self.battery_low_threshold:
                return self.log.error("Battery high threshold is lower than battery low threshold")
            if self.battery_high_threshold * 0.1 > self.battery_low_threshold:
                self.log.info("Battery low threshold is lower than 10% of battery high threshold")
                self.battery_low_threshold = self.battery_high_threshold * 0.1

            if entry["Vcap"] > self.battery_high_threshold:
                    return self.img_battery_100_percent
            elif entry["Vcap"] > self.battery_high_threshold * 0.75:
                    return self.img_battery_75_percent
            elif entry["Vcap"] > self.battery_high_threshold * 0.50:
                    return self.img_battery_50_percent
            elif entry["Vcap"] > self.battery_high_threshold * 0.25:
                    return self.img_battery_25_percent
            elif entry["Vcap"] > self.battery_high_threshold * 0.10:        
                return self.img_battery_10_percent
            elif entry["Vcap"] < self.battery_low_threshold:
                return self.img_battery_0_percent
    
    def select_temperature_images(self,entry):   
        if entry["Temperature"] == None:
            return self.img_temp_low_power , self.img_temp_low_power
        else:
            #sanity check
            if self.temperature_high_threshold < self.temperature_low_threshold:
                return self.log.error("Temperature high threshold is lower than temperature low threshold")
            if entry["Temperature"] > self.temperature_high_threshold:
                return self.img_hot_active, self.img_cold_notactive
            elif entry["Temperature"] >= self.temperature_low_threshold and entry["Temperature"] <= self.temperature_high_threshold:
                return self.img_hot_notactive, self.img_cold_notactive
            elif entry["Temperature"] < self.temperature_low_threshold:
                return self.img_hot_notactive, self.img_cold_active
    
    def dummy_AssetTagData(self):
        """
        Generates dummy data for AssetTagData.
        This function generates dummy data for the AssetTagData dictionary in the following format:
        {
            "mac_address_1": [
                {
                    "devId": "f<random_number>",
                    "timestamp": "<random_date> <random_time>",
                    "MAC": "mac_address_1",
                    "Raw data": "02010618ffff0205187a6525686bf3e10a380bfeff003ce803000004",
                    "payloadLen": <random_number>,
                    "Temperature": <random_temperature>,
                    "Vcap": <random_voltage>,
                    "deltaVcap": <random_delta_voltage>,
                    "Intensity": <random_intensity>,
                    "Power": <random_power>,
                    "Next": <random_next>,
                    "Mode": <random_mode>,
                    "State": <random_power_state>
                },
                ...
            ],
            "mac_address_2": [
                ...
            ],
            ...
        }
        The function first checks if the AssetTagData dictionary exists. If not, it creates it.
        It then creates a list for each MAC address in the macAddressList.
        For each MAC address, it creates a dictionary with dummy data and appends it to the corresponding list.
        Parameters:
        - self: The instance of the class.
        Returns:
        None
        """
        ## GUI visualization parameters
        BATTERY_HIGH_VCAP = 2500    # mV
        BATTERY_LOW_VCAP = 1200     # mV
        TEMP_HIGH = 25              # °C
        TEMP_LOW = 4                # °C
        power_states = [ "POWER_LEVEL_SKIP_2ND", "POWER_LEVEL_MIN", "POWER_LEVEL_MEDIUM", "POWER_LEVEL_HIGH", "POWER_LEVEL_MAX" ]
        current_mac = ""

        #if self.pvassttagapp.AssetTagData is not exist, create it
        if not self.pvassttagapp.AssetTagData:
            self.pvassttagapp.AssetTagData = dict()
        
        macAddressList = ["f8:44:77:aa:ac:76","f8:44:77:aa:ac:74","f8:44:77:aa:ac:75"]
        
        #create list for every mac address
        for address in macAddressList:
            if address not in self.pvassttagapp.AssetTagData.keys():
                self.pvassttagapp.AssetTagData[address] = []
        
        for current_mac in macAddressList:
            tempDict = dict()
            tempDict["devId"] = "f"+str(random.randint(0, 100))
            tempDict["timestamp"] = "28/08/2024 "+ str(random.randint(0,24))+":"+ str(random.randint(0,59))+":"+ str(random.randint(0,59))
            tempDict["MAC"] = current_mac
            tempDict["Raw data"] = "02010618ffff0205187a6525686bf3e10a380bfeff003ce803000004"
            tempDict["payloadLen"] = random.randint(0, 100)
            tempDict["Temperature"] = random.randint(TEMP_LOW-4, TEMP_HIGH+10)
            tempDict["Vcap"] = random.randint(BATTERY_LOW_VCAP-500, BATTERY_HIGH_VCAP+500)
            tempDict["deltaVcap"] = random.randint(-100, 100)
            tempDict["Intensity"] = random.randint(0, 100)
            tempDict["Power"] = random.randint(0.0,10)
            tempDict["Next"] = random.randint(0, 100)
            tempDict["Mode"] = random.randint(0, 10)
            tempDict["State"] = power_states[random.randint(0, 4)]
            self.pvassttagapp.AssetTagData[current_mac].append(tempDict)
        #print (self.pvassttagapp.AssetTagData)  


    '''

    Old MEthod for refresh_table keeping it for the sake of reference
    def refresh_table(self):
        for macAddress, data in self.pvassttagapp.AssetTagData.items():
            # first line
            print (f" self.frame2.winfo_children(): ", len(self.frame2.winfo_children()))
            if len(self.frame2.winfo_children()) <= 7 and macAddress not in self.gui_table.keys():
                self.gui_table[macAddress] = 1
                for entry in data:
                    #Device ID
                    row_devid = CustomCell(self.frame2,background="white", text=entry["devId"], name=f"devid_{self.gui_table[macAddress]}")
                    row_devid.grid(row=self.gui_table[macAddress], column=0, sticky="ew")
                    #Timestamp
                    row_timestamp = CustomCell(self.frame2,background="white", text=entry["timestamp"], name=f"timestamp_{self.gui_table[macAddress]}")
                    row_timestamp.grid(row=self.gui_table[macAddress], column=1, sticky="ew")
                    #MAC address
                    row_MAC = CustomCell(self.frame2,background="white", text=entry["MAC"], name=f"mac_{self.gui_table[macAddress]}")
                    row_MAC.grid(row=self.gui_table[macAddress], column=2, sticky="ew")
                    #Energy level
                    row_energy_level = CustomCell(self.frame2,background="white", image = self.select_battery_images(entry), name=f"battery_{self.gui_table[macAddress]}")
                    row_energy_level.grid(row=self.gui_table[macAddress], column=3, sticky="ew")
                    #Temperature
                    row_Temperature_high,row_Temperature_low = self.select_temperature_images(entry)
                    row_Temperature_high = CustomCell(self.frame2,background="white", image = row_Temperature_high, name=f"temperature_high_{self.gui_table[macAddress]}")
                    row_Temperature_high.grid(row=self.gui_table[macAddress], column=4, sticky="ew")
                    row_Temperature_low = CustomCell(self.frame2,background="white", image = row_Temperature_low, name=f"temperature_low_{self.gui_table[macAddress]}")
                    row_Temperature_low.grid(row=self.gui_table[macAddress], column=5, sticky="ew")
            elif len(self.frame2.winfo_children()) > 7 and macAddress in self.gui_table.keys():
                print (f" self.gui_table.keys(): ", self.gui_table.keys())
                print (f"devid:" , macAddress)
                for macAddress_row in self.gui_table.values():
                    #print (devid_row)
                    #print (data)
                    for widget in self.frame2.winfo_children():
                        if widget.grid_info()["row"]  ==  macAddress_row:
                            for entry in data:
                                #Device ID
                                if widget.winfo_name() == f"devid_{macAddress_row}":
                                    widget.configure(text=entry["devId"])
                                #Timestamp
                                if widget.winfo_name()  ==  f"timestamp_{macAddress_row}":
                                    #print(widget.winfo_name())
                                    widget.configure(text=entry["timestamp"])
                                #MAC address
                                if widget.winfo_name()  ==  f"mac_{macAddress_row}":
                                    widget.configure(text=entry["MAC"])
                                #Energy level
                                if widget.winfo_name()  ==  f"battery_{macAddress_row}":
                                    widget.configure(image=self.select_battery_images(entry))
                                #Temperature
                                if widget.winfo_name()  ==  f"Temperature_high_{macAddress_row}":
                                    widget.configure(image=self.select_temperature_images(entry)[0])
                                if widget.winfo_name()  ==  f"Temperature_low_{macAddress_row}":
                                    widget.configure(image=self.select_temperature_images(entry)[1])                    
            #add new Device ID rows
            elif len(self.frame2.winfo_children()) > 7 and macAddress not in self.gui_table.keys():
                print("hey new row")
                for macAddress_row in self.gui_table.values():
                    #print (devid_row)
                    #print (data)
                    for entry in data:
                        #Device ID
                        row_devid = CustomCell(self.frame2,background="white", text=entry["devId"], name=f"devid_{self.gui_table[macAddress]}")
                        row_devid.grid(row = self.gui_table[macAddress_row] ,  column=0, sticky="ew")
                        #Timestamp
                        row_timestamp = CustomCell(self.frame2,background="white", text=entry["timestamp"], name=f"timestamp_{self.gui_table[macAddress]}")
                        row_timestamp.grid(row = self.gui_table[macAddress_row] ,  column=1, sticky="ew")
                        #MAC address
                        row_MAC = CustomCell(self.frame2,background="white", text=entry["MAC"], name=f"mac_{self.gui_table[macAddress]}")
                        row_MAC.grid(row = self.gui_table[macAddress_row] ,  column=2, sticky="ew")
                        #Energy level
                        row_energy_level = CustomCell(self.frame2, image = self.select_battery_images(entry),name=f"battery_{self.gui_table[macAddress]}",background="white")
                        row_energy_level.grid(row = self.gui_table[macAddress_row] ,  column=3, sticky="ew")
                        #Temperature
                        row_Temperature_high,row_Temperature_low = self.select_temperature_images(entry)
                        row_Temperature_high = CustomCell(self.frame2,background="white", image = row_Temperature_high, name=f"temperature_high_{self.gui_table[macAddress]}")
                        row_Temperature_high.grid(row=self.gui_table[macAddress_row], column=4, sticky="ew")
                        row_Temperature_low = CustomCell(self.frame2,background="white", image = row_Temperature_low, name=f"temperature_low_{self.gui_table[macAddress]}")
                        row_Temperature_low.grid(row=self.gui_table[macAddress_row], column=5, sticky="ew")
                self.gui_table[macAddress] = len(self.gui_table.keys()) + 1
                '''
    def table_new_row(self,lastrow,rowData):
        target_row = lastrow + 1
        '''
        print(f"Target row: {target_row}")
        print(f"Last row: {lastrow}")
        print(f'Row data: {rowData}')
        '''
        try:
            macAddress = rowData[0]["MAC"]
        except(IndexError, KeyError) as e:       
            self.log.info("MAC address is missing in the data entry possible low power mode: {e}")
            return
        
        if macAddress not in self.gui_table.keys():
            self.gui_table[macAddress] = target_row
        
        for entry in rowData:
            #Device ID
            row_devid = CustomCell(self.frame2,background="white", text=entry["devId"], name=f"devid_{self.gui_table[macAddress]}")
            row_devid.grid(row = target_row ,  column=0, sticky="ew")
            #Timestamp
            row_timestamp = CustomCell(self.frame2,background="white", text=entry["timestamp"], name=f"timestamp_{self.gui_table[macAddress]}")
            row_timestamp.grid(row = target_row ,  column=1, sticky="ew")
            #MAC address
            row_MAC = CustomCell(self.frame2,background="white", text=entry["MAC"], name=f"mac_{self.gui_table[macAddress]}")
            row_MAC.grid(row = target_row ,  column=2, sticky="ew")
            #Energy level
            row_energy_level = CustomCell(self.frame2, image = self.select_battery_images(entry),name=f"battery_{self.gui_table[macAddress]}",background="white")
            row_energy_level.grid(row = target_row ,  column=3, sticky="ew")
            #Temperature
            row_Temperature_high,row_Temperature_low = self.select_temperature_images(entry)
            row_Temperature_high = CustomCell(self.frame2,background="white", image = row_Temperature_high, name=f"temperature_high_{self.gui_table[macAddress]}")
            row_Temperature_high.grid(row=target_row, column=4, sticky="ew")
            row_Temperature_low = CustomCell(self.frame2,background="white", image = row_Temperature_low, name=f"temperature_low_{self.gui_table[macAddress]}")
            row_Temperature_low.grid(row=target_row, column=5, sticky="ew")
        
    def table_update_row(self,rowData):
        
        try:
            macAddress = rowData[0]["MAC"]
        except(IndexError, KeyError) as e:       
            self.log.info("MAC address is missing in the data entry possible low power mode: {e}")
            return
        
        current_row = self.gui_table[macAddress]
        for widget in self.frame2.winfo_children():
            if widget.grid_info()["row"]  ==  current_row:
                for entry in rowData:
                    #Device ID
                    if widget.winfo_name() == f"devid_{current_row}":
                        widget.configure(text=entry["devId"])
                    #Timestamp
                    if widget.winfo_name()  ==  f"timestamp_{current_row}":
                        #print(widget.winfo_name())
                        widget.configure(text=entry["timestamp"])
                    #MAC address
                    if widget.winfo_name()  ==  f"mac_{current_row}":
                        widget.configure(text=entry["MAC"])
                    #Energy level
                    if widget.winfo_name()  ==  f"battery_{current_row}":
                        widget.configure(image=self.select_battery_images(entry))
                    #Temperature
                    if widget.winfo_name()  ==  f"temperature_high_{current_row}":
                        widget.configure(image=self.select_temperature_images(entry)[0])
                    if widget.winfo_name()  ==  f"temperature_low_{current_row}":
                        widget.configure(image=self.select_temperature_images(entry)[1])     

    def refresh_table(self):
        #print (f" self.frame2.winfo_children(): ", len(self.frame2.winfo_children()))
        for macAddress in self.pvassttagapp.AssetTagData.keys():
            if not self.pvassttagapp.AssetTagData[macAddress] == {}:
                rowData = self.pvassttagapp.AssetTagData[macAddress]
                if len(self.frame2.winfo_children()) <= 7:
                    self.table_new_row(0,rowData)
                else:
                    if macAddress in self.gui_table.keys():
                        self.table_update_row(rowData)
                    else:
                        self.table_new_row(len(self.gui_table.keys()),rowData)


    def display_debug(self):
        self.scrolled_text.delete(1.0, tk.END)
        MAX_ENTRIES = 100  
        # Collect all entries
        all_entries = []
        for macAddress, data in self.pvassttagapp.AssetTagData.items():
            for entry in data:
                all_entries.append(entry)
        
        # Sort entries by timestamp
        all_entries.sort(key=lambda x: x['timestamp'])
        
        # Select the last MAX_ENTRIES 
        last_entries = all_entries[-MAX_ENTRIES:]
        
        # Display the last 100 entries
        for entry in last_entries:
            self.scrolled_text.insert(tk.END, f"Device ID: {entry['devId']} ")
            self.scrolled_text.insert(tk.END, f"Timestamp: {entry['timestamp']}\n")
            self.scrolled_text.insert(tk.END, f" MAC: {entry['MAC']} \n ")
            self.scrolled_text.insert(tk.END, f" Raw data: {entry['Raw data']}")
            self.scrolled_text.insert(tk.END, f" Payload length: {entry['payloadLen']}\n")
            self.scrolled_text.insert(tk.END, f" Temperature: {entry['Temperature']} °C")
            self.scrolled_text.insert(tk.END, f" Vcap: {entry['Vcap']} mV")
            self.scrolled_text.insert(tk.END, f" deltaVcap: {entry['deltaVcap']} mV")
            self.scrolled_text.insert(tk.END, f" Intensity: {entry['Intensity']}")
            self.scrolled_text.insert(tk.END, f" Power: {entry['Power']} dBm")
            self.scrolled_text.insert(tk.END, f" Next: {entry['Next']} ms")
            self.scrolled_text.insert(tk.END, f" Mode: {entry['Mode']}")
            self.scrolled_text.insert(tk.END, f" State: {entry['State']} \n \n")
            self.scrolled_text.insert(tk.END, "\n")
        
        self.scrolled_text.see(tk.END)
if __name__ == "__main__":
    thresholds = [2500, 1200, 32, 4]
    gui = GUIApp(thresholds)
    gui.run()