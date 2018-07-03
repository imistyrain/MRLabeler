#coding=utf-8
#
#   数据集辅助检查和绘制工具
#
#   版本V1.0.1 2018.3.15
#
#   烟雨@触控科技
#
#如果导入tqm出错,请pip install tqdm
#
import os,cv2,shutil,argparse,random
from xml.etree import ElementTree as ET
from tqdm import tqdm

#数据集组织结构如下:
#--datset_rootdir:CK2018
#   --images:原始图片存放目录,后缀务必为.jpg
#   --Annotations:标注文件存放目录,文件名和原始图片相同,后缀为xml,VOC标注,
#                 坐标务必为整数,范围(0 - W-1,0 - H-1),W和W分别为图片的宽和高
#   --ImageSets:切分训练集和测试集的描述文件
#       --Main
#           train.txt
#           val.txt
#           trainval.txt
#           test.txt
#   可选
#   mrconfig.xml:数据集描述文件,参见https://github.com/imistyrain/MRLabeler
#   --gt:绘制的真值图存放文件夹
#   --labels:yolo训练所需标注格式
#   trainval.txt:yolo训练所需训练集描述文件
#   test.txt:yolo训练所需测试集描述文件

#各类标注错误存放文件夹
noannoimgdir="noannoimgs"#图片没有对应的标注
noannodir="noannos"#标注没有对应的图片
exterrordir="exterrors"#后缀名不是args.ext=.jpg

#检查文件夹是否存在，如果不存在则新建
def mkr(dir):
    if not os.path.exists(dir):
        os.makedirs(dir)

#滤除后缀名不是args.ext=.jpg的图片
def checkext(args):
    print("checking ext errors")
    rootdir=args.dataset_dir
    img_dir=rootdir+"/"+args.images_dir
    files=os.listdir(img_dir)
    for file in tqdm(files):
        if os.path.splitext(file)[1]!=args.ext:
            print(file)
            img_path=img_dir+"/"+file
            ext_path=exterrordir+"/"+file
            shutil.move(img_path,ext_path)
    print("checking ext errors done")

#滤除没有标注的图片
def filtererrorsinimages(args):
    rootdir=args.dataset_dir
    Anno_dir=rootdir+"/"+args.annos_dir
    img_dir=rootdir+"/"+args.images_dir
    files=os.listdir(img_dir)
    for file in tqdm(files):
        img_path=img_dir+"/"+file
        anno_path=Anno_dir+"/"+os.path.splitext(file)[0]+".xml"
        if not os.path.exists(anno_path):
            noanoimg_path=noannoimgdir+"/"+file
            shutil.move(img_path,noanoimg_path)

#滤除没有图片的标注
def filtererrorinannotations(args):
    rootdir=args.dataset_dir
    Anno_dir=rootdir+"/"+args.annos_dir
    img_dir=rootdir+"/"+args.images_dir
    files=os.listdir(Anno_dir)
    for file in tqdm(files):
        anno_path=Anno_dir+"/"+file
        img_path=img_dir+"/"+os.path.splitext(file)[0]+args.ext
        if not os.path.exists(img_path):
            noano_path=noannodir+"/"+file
            shutil.move(anno_path,noano_path)

#将标注里的坐标转换为整数
def check_annotations(args):
    print("checking annotations")
    rootdir=args.dataset_dir
    Anno_dir=rootdir+"/"+args.annos_dir
    img_dir=rootdir+"/"+args.images_dir
    files=os.listdir(Anno_dir)
    for file in tqdm(files):
        anno_path=Anno_dir+"/"+file
        img_path=img_dir+"/"+os.path.splitext(file)[0]+args.ext
        img=cv2.imread(img_path)
        if not img.data:
            print(img_path+" dose not exist")
            continue
        h,w,c=img.shape
        annoxml=ET.parse(anno_path) 
        objects=annoxml.findall('object')
        for i in range(len(objects)):
            object=objects[i]
            bndbox=object.find("bndbox")
            xmin=(int)((float)(bndbox.find("xmin").text))
            xmax=(int)((float)(bndbox.find("xmax").text))
            ymin=(int)((float)(bndbox.find("ymin").text))
            ymax=(int)((float)(bndbox.find("ymax").text))
            if xmin<0:
                xmin=0
            if ymin<0:
                ymin=0
            if xmax>=w-1:
                xmax=w-1
            if ymax>=h-1:
                ymax=h-1
            bndbox.find("xmin").text=str(xmin)
            bndbox.find("xmax").text=str(xmax)
            bndbox.find("ymin").text=str(ymin)
            bndbox.find("ymax").text=str(ymax)
        annoxml.write(anno_path)
    print("checking annotations done")

#滤除数据集中的错误
def filtererrors(args):
    mkr(noannoimgdir)
    mkr(noannodir)
    mkr(exterrordir)
    print("checking errors")
    try:
        checkext(args)
        filtererrorsinimages(args)
        filtererrorinannotations(args)
        #check_annotations(args)
    except FileNotFoundError as e:
        print(e)
    print("checking done")

def get_random_color(pastel_factor = 0.5):
    return [256*(x+pastel_factor)/(1.0+pastel_factor) for x in [random.uniform(0,1.0) for i in [1,2,3]]]

def color_distance(c1,c2):
    return sum([abs(x[0]-x[1]) for x in zip(c1,c2)])

def generate_new_color(existing_colors,pastel_factor = 0.5):
    max_distance = None
    best_color = None
    for i in range(0,100):
        color = get_random_color(pastel_factor = pastel_factor)
        if not existing_colors:
            return color
        best_distance = min([color_distance(color,c) for c in existing_colors])
        if not max_distance or best_distance > max_distance:
            max_distance = best_distance
            best_color = color
    return best_color

#get all annotation labels and distribution
def get_all_labels(args):
    print("get all label distributions")
    dataset_dir=args.dataset_dir
    Annotation_dir=dataset_dir+"/"+args.annos_dir
    images_dir=dataset_dir+"/"+args.images_dir
    gt_dir=dataset_dir+"/"+args.gt_dir
    if args.save_gt:
        mkr(gt_dir)
    files=os.listdir(Annotation_dir)
    labeldistributions={}
    total=0
    colors=dict()
    #for file in files:
    for file in tqdm(files):
        annopath=Annotation_dir+"/"+file
        annoxml=ET.parse(annopath) 
        objects=annoxml.findall('object')
        imgfilename=os.path.splitext(file)[0]+args.ext
        if args.show_annotations or args.save_gt:
            imagepath=images_dir+"/"+imgfilename
            image=cv2.imread(imagepath)
            if image is None:
                continue
        names=set()
        objs=dict()
        for i in range(len(objects)):
            total=total+1
            name=objects[i].find("name").text
            names.add(name)
            if name in labeldistributions:
                labeldistributions[name]=labeldistributions[name]+1
            else:
                labeldistributions[name]=1
            rect=list()
            bndbox=objects[i].find("bndbox")
            for i in range(len(bndbox)):
                rect.append(float(bndbox[i].text))
            if name in objs:
                objs[name].append(rect)
            else:
                objs[name]=list()
                objs[name].append(rect)
        if args.show_annotations or args.save_gt:
            for name in names:
                if name in colors:
                    color=colors[name]
                else:
                    colors[name]=generate_new_color(None,pastel_factor = 0.9)
                color=colors[name]
            if args.save_classified:
                if args.show_alllabels:
                    for name in names:
                        for rect in objs[name]:
                            cv2.rectangle(image,(int(rect[0]),int(rect[1])),(int(rect[2]),int(rect[3])),color)
                            if rect[1]<20:
                                cv2.putText(image,name,(int(rect[0]),20),1,1,color)
                            else:
                                cv2.putText(image,name,(int(rect[0]),int(rect[1])),3,1,color)
                    for name in names:    
                        save_class_dir=gt_dir+"/"+name
                        mkr(save_class_dir)
                        gt_path=save_class_dir+"/"+imgfilename
                        cv2.imwrite(gt_path,image)
                else:
                    for name in names:
                        color=colors[name]
                        show=image.copy()
                        for rect in objs[name]:
                            if rect[1]<20:
                                cv2.putText(show,name,(int(rect[0]),20),1,1,color)
                            else:
                                cv2.putText(show,name,(int(rect[0]),int(rect[1])),3,1,color)
                            cv2.rectangle(show,(int(rect[0]),int(rect[1])),(int(rect[2]),int(rect[3])),color)
                            save_class_dir=gt_dir+"/"+name
                            mkr(save_class_dir)
                            gt_path=save_class_dir+"/"+imgfilename
                            cv2.imwrite(gt_path,show)
         
            else:
                for name in names:
                    color=colors[name]
                    for rect in objs[name]:
                        if rect[1]<20.0:
                            cv2.putText(image,name,(int(rect[0]),20),1,1,color)
                        else:
                            cv2.putText(image,name,(int(rect[0]),int(rect[1])),3,1,color)
                        cv2.rectangle(image,(int(rect[0]),int(rect[1])),(int(rect[2]),int(rect[3])),color)
                gt_path=gt_dir+"/"+imgfilename
                cv2.imwrite(gt_path,image)
        if args.show_annotations:
            cv2.imshow("gt",image)
            cv2.waitKey(1)
    with open(dataset_dir+"/labelsdistribution.txt","w")as f:
        #for s in labelset:
        #    f.write(str(s)+"\n")
        for index,d in enumerate(labeldistributions):
            line=str(index)+"\t"+str(d)+":"+str(labeldistributions[d])+"\t"+str(labeldistributions[d]/total)
            f.write(line+"\n")
            print(line)

#切分训练集和测试集,同时生成VOC和yolo格式
def split_traintest(args,trainratio=0.7,valratio=0.2,testratio=0.1):
    print("spliting train / test")
    dataset_dir=args.dataset_dir
    annodir=dataset_dir+"/"+args.images_dir
    files=os.listdir(annodir)
    trains=[]
    vals=[]
    trainvals=[]
    tests=[]
    random.shuffle(files)
    for i in range(len(files)):
        filepath=dataset_dir+"/images/"+files[i][:-3]+args.ext
        if(i<trainratio*len(files)):
            trains.append(files[i])
            trainvals.append(files[i])
        elif i<(trainratio+valratio)*len(files):
            vals.append(files[i])
            trainvals.append(files[i])
        else:
            tests.append(files[i])
    #生成yolo所需的训练集和测试集描述文件
    with open(dataset_dir+"/trainval.txt","w")as f:
        for line in trainvals:
            line=dataset_dir+"/"+args.images_dir+"/"+line
            f.write(line+"\n")
    with open(dataset_dir+"/test.txt","w") as f:
        for line in tests:
            line=dataset_dir+"/"+args.images_dir+"/"+line
            f.write(line+"\n")
    #生成VOC格式的训练集和测试集描述文件
    maindir=dataset_dir+"/"+"ImageSets/Main"
    mkr(maindir)
    with open(maindir+"/train.txt","w") as f:
        for line in trains:
            line=line[:line.rfind(".")]
            f.write(line+"\n")
    with open(maindir+"/val.txt","w") as f:
        for line in vals:
            line=line[:line.rfind(".")]
            f.write(line+"\n")
    with open(maindir+"/trainval.txt","w") as f:
        for line in trainvals:
            line=line[:line.rfind(".")]
            f.write(line+"\n")
    with open(maindir+"/test.txt","w") as f:
        for line in tests:
            line=line[:line.rfind(".")]
            f.write(line+"\n")
    print("spliting done")

#获取清洗后的真值文件夹包含的文件列表
def get_reserve_list(dir):
    all_files=[]
    subdirs=os.listdir(dir)
    all_files=set()
    for sub in subdirs:
        subdir=dir+"/"+sub
        if os.path.isdir(subdir):
            files=os.listdir(subdir)
            for file in files:
                all_files.add(file)
    return all_files

#将不在保留中的文件移至另外的文件夹存放
def remove_file_in_dir(src_dir,removed_dir,reserve_files):
    files=os.listdir(src_dir)
    for file in tqdm(files):
        if not file in reserve_files:
            print(file)
            src_path=src_dir+"/"+file
            dst_path=removed_dir+"/"+file
            shutil.move(src_path,dst_path)

#由清洗后的真值文件夹重新
def remove_files_not_in_gt(args):
    print("removing files not in gt dir")
    img_dir=args.dataset_dir+"/"+args.images_dir
    annodir=args.dataset_dir+"/"+args.annos_dir
    gtdir=args.dataset_dir+"/"+args.gt_dir
    #all_files=get_reserve_list(gtdir)
    all_files=os.listdir(gtdir)
    removeddir="removed"
    mkr(removeddir)
    remove_file_in_dir(img_dir,removeddir,all_files)
    all_files=[af[:-3]+"xml" for af in all_files]
    remove_file_in_dir(annodir,removeddir,all_files)
    print("removing files not in gt dir done")
#获取输入的参数
def get_args():
    parser = argparse.ArgumentParser()
    #设置数据集的文件夹所在目录
    parser.add_argument('--dataset_dir', type=str,help='Path to the data directory of dataset.',
                        default=
                        #"E:/Detection/Datasets/voc/VOCdevkit/VOC2007"
                        "D:/Detection/CKdemo/CK2018"
                        )
    parser.add_argument('--show_annotations', type=bool,help='show annotations on label analysis',default=False)
    parser.add_argument('--show_alllabels', type=bool,help='whether show all labels',default=True)
    parser.add_argument('--save_gt', type=bool,help='classified show gt',default=True)
    parser.add_argument('--gt_dir', type=str,help='gt dir',default="gt")
    parser.add_argument('--save_classified', type=bool,help='classified show gt',default=False)

    parser.add_argument('--images_dir', type=str,help='dataset image file dir',default=
                        "images"
                        #"JPEGImages"
                        )
    parser.add_argument('--annos_dir', type=str,help='annotations dir',default="Annotations")
    parser.add_argument('--ext', type=str,help='dataset image file extention',default=".jpg")

    return parser.parse_args()

#主函数
def main():
    args=get_args()
    gt_dir=args.dataset_dir+"/"+args.gt_dir
    if os.path.exists(gt_dir):
        remove_files_not_in_gt(args)
    #检查所有已知错误类型
    filtererrors(args)
    #切分训练集和测试集
    split_traintest(args)
    #统计标签分布，展示和绘制真值图
    get_all_labels(args)

if __name__=="__main__":
    main()