#coding=utf-8
import os,shutil
#如果导入这句报错的话,pip install tqdm
from tqdm import tqdm
from xml.etree import ElementTree as ET

#fddb文件夹路径
fddb_dir="D:/Face/Datasets/fddb"
#自建文件夹路径
dst_dir="D:/Detection/CKdemo/CK2018"

#下面的相对路径不用动
fddb_img_dir=fddb_dir+"/images"
fddb_anno_dir=fddb_dir+"/Annotations"
dst_img_dir=dst_dir+"/images"
dst_anno_dir=dst_dir+"/Annotations"

def mkr(dir):
    if not os.path.exists(dir):
        os.makedirs(dir)

def check_copy_files(file):
    global fddb_img_dir,fddb_anno_dir,dst_img_dir,dst_anno_dir
    max_faces=6
    min_pixels=60
    img_path=fddb_img_dir+"/"+file
    dst_img_path=dst_img_dir+"/"+file
    anno_path=fddb_anno_dir+"/"+file[:-3]+"xml"
    dst_anno_path=dst_anno_dir+"/"+file[:-3]+"xml"
    annoxml=ET.parse(anno_path)
    objects=annoxml.findall('object')
    if len(objects)>max_faces:
        print(file+"has more than "+str(max_faces))
        return -1
    else:
        bHasbadface=False
        for object in objects:
            bndbox=object.find("bndbox")
            xmin=(float)(bndbox.find("xmin").text)
            ymin=(float)(bndbox.find("ymin").text)
            xmax=(float)(bndbox.find("xmax").text)
            ymax=(float)(bndbox.find("ymax").text)
            width=xmax-xmin
            height=ymax-ymin
            if width<min_pixels or height<min_pixels:
                bHasbadface=True
                break
        if bHasbadface:
            print(file+" has faces less than "+str(min_pixels))
            return -2
        else:
            shutil.copy(img_path,dst_img_path)
            shutil.copy(anno_path,dst_anno_path)    
    return 0

def get_faces_fddb(num=10000):
    num_abandoned=0
    mkr(dst_img_dir)
    mkr(dst_anno_dir)
    files=os.listdir(fddb_img_dir)
    index=0
    for file in tqdm(files):
        if check_copy_files(file)==0:
            index+=1
            if index>=num:
                break
        else:
            num_abandoned+=1
    print(str(num_abandoned)+" has been abandoned")

if __name__=="__main__":
    get_faces_fddb()