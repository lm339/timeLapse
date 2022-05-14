# Copyright (c) 2020 Ultimaker B.V.
# Cura is released under the terms of the LGPLv3 or higher.
# Created by Wayne Porter

from ..Script import Script

def checkLayerNumber(line):    
    key  = 0
    start = 0
    strlen = 0
    for i in line:
        if key == 0:             
            if str.isdigit(i):
                key = 1         
            else:
                start = start+1
        if key ==1:
            if str.isdigit(i):
               strlen  = strlen+1          
            else:
                break
    print("strlen ",strlen,start)
    if strlen != 0:
        return (int(line[start:start+strlen]))   
    return 1

class TimeLapse(Script):
    def __init__(self):
        super().__init__()

    def getSettingDataString(self):
        return """{
            "name": "Time Lapse",
            "key": "TimeLapse",
            "metadata": {},
            "version": 2,
            "settings":
            {
                "disable":
                {
                    "label": "关闭",
                    "description": "勾选后该脚本失效，不对GCODE文件做任何修改，这个是我自己加的，有些模型不需要延时摄影的时候只需要勾选，下次不用重新设置",
                    "type": "bool",
                   "default_value": false
                },       
                
                "trigger_command":
                {
                    "label": "触发相机命令",
                    "description": "触发相机拍照的GCODE命令，我们用不上，我们用微动开关触发",
                    "type": "str",
                    "default_value": "M240"
                },
                
                "pause_length":
                {
                    "label": "暂停时间",
                    "description": "移动到指定位置后停留时间，给相机留拍照时间",
                    "type": "int",
                    "default_value": 700,
                    "minimum_value": 0,
                    "unit": "ms"
                },
                "park_print_head":
                {
                    "label": "是否把喷头移到指定位置",
                    "description": "触发拍照前是否要把喷头放到指定位置，当然要移。",
                    "type": "bool",
                    "default_value": true
                },
                "head_park_x":
                {
                    "label": "X轴位置",
                    "description": "X轴位置，移开，挡到拍照了",
                    "unit": "mm",
                    "type": "float",
                    "default_value": 0,
                    "enabled": "park_print_head"
                },
                "head_park_y":
                {
                    "label": "Y轴位置",
                    "description": "放到指定位置，触发微动开关",
                    "unit": "mm",
                    "type": "float",
                    "default_value": 220,
                    "enabled": "park_print_head"
                },
                "park_feed_rate":
                {
                    "label": "移动速度",
                    "description": "移动速度，不丢步的情况下越快越好",
                    "unit": "mm/s",
                    "type": "float",
                    "default_value": 9000,
                    "enabled": "park_print_head"
                },
                "retract":
                {
                    "label": "E轴回抽距离",
                    "description": "E轴回抽，防止拉丝",
                    "unit": "mm",
                    "type": "int",
                    "default_value": 10
                },
                "zhop":
                {
                    "label": "Z轴位置",
                    "description": "Z轴位置，影响不大，我们这里不用管，默认就好",
                    "unit": "mm",
                    "type": "float",
                    "default_value": 0
                }
            }
        }"""
        
    def execute(self, data):
        feed_rate = self.getSettingValueByKey("park_feed_rate")
        park_print_head = self.getSettingValueByKey("park_print_head")
        x_park = self.getSettingValueByKey("head_park_x")
        y_park = self.getSettingValueByKey("head_park_y")
        trigger_command = self.getSettingValueByKey("trigger_command")
        disable = self.getSettingValueByKey("disable")
        pause_length = self.getSettingValueByKey("pause_length")
        retract = int(self.getSettingValueByKey("retract"))
        zhop = self.getSettingValueByKey("zhop")
        gcode_to_append = ";TimeLapse Begin\n"
        last_x = 0
        last_y = 0
        last_z = 0
        if disable:
            return data
            
        if park_print_head:
            gcode_to_append += self.putValue(G=1, F=feed_rate,
                                             X=x_park, Y=y_park) + " ;Park print head\n"
        gcode_to_append += self.putValue(M=400) + " ;Wait for moves to finish\n"
        gcode_to_append += trigger_command + " ;Snap Photo\n"
        gcode_to_append += self.putValue(G=4, P=pause_length) + " ;Wait for camera\n"

        for idx, layer in enumerate(data):
            for line in layer.split("\n"):
                if self.getValue(line, "G") in {0, 1}:  # Track X,Y,Z location.
                    last_x = self.getValue(line, "X", last_x)
                    last_y = self.getValue(line, "Y", last_y)
                    last_z = self.getValue(line, "Z", last_z)
            # Check that a layer is being printed
            lines = layer.split("\n")
            for line in lines:
                if ";LAYER:" in line:


                    if retract != 0: # Retract the filament so no stringing happens
                        layer += self.putValue(M=83) + " ;Extrude Relative\n"
                        layer += self.putValue(G=1, E=-retract, F=3000) + " ;Retract filament\n"
                        layer += self.putValue(M=82) + " ;Extrude Absolute\n"
                        layer += self.putValue(M=400) + " ;Wait for moves to finish\n" # Wait to fully retract before hopping

                    if zhop != 0:
                        layer += self.putValue(G=1, Z=last_z+zhop, F=3000) + " ;Z-Hop\n"

                    layer += gcode_to_append

                    if(checkLayerNumber(line) == 0):
                        layer += self.putValue(G=4, P=10000 + len(data)) + " ;Wait start \n"

                    if zhop != 0:
                        layer += self.putValue(G=0, X=last_x, Y=last_y, Z=last_z) + "; Restore position \n"
                    else:
                        layer += self.putValue(G=0, X=last_x, Y=last_y) + "; Restore position \n"

                    if retract != 0:
                        layer += self.putValue(M=400) + " ;Wait for moves to finish\n"
                        layer += self.putValue(M=83) + " ;Extrude Relative\n"
                        layer += self.putValue(G=1, E=retract, F=3000) + " ;Retract filament\n"
                        layer += self.putValue(M=82) + " ;Extrude Absolute\n"

                    data[idx] = layer
                    break
            if(idx+1 == len(data)):
                data[idx] += self.putValue(G=1, F=feed_rate, X=x_park, Y=y_park) + "\n"
                data[idx] += self.putValue(M=400) + " ;Wait for moves to finish\n"
                data[idx] += self.putValue(G=4, P=5000 + len(data)) + "\n"
                data[idx] += self.putValue(G=1, F=feed_rate, X=x_park, Y=180) +"\n"  
                data[idx] += self.putValue(M=84)  + "/n;print end power by lm339\n"
        return data
