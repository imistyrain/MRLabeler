#coding=utf-8
#给定指定数据集标注文件，统计其标签及分布并画出标注
import os,cv2,shutil,argparse
from xml.etree import ElementTree as ET
from tqdm import tqdm

images_dir_rel="images"#"JPEGImages"#
image_ext="jpg"#"png"
noannoimgdir="noannoimgs"
noannodir="noanno"
def mkr(dir):
    if not os.path.exists(dir):
        os.makedirs(dir)
#get images that has annotaions
def filtererrorsinimages(args):
    rootdir=args.dataset_dir
    Anno_dir=rootdir+"/"+"Annotations"
    img_dir=rootdir+"/"+"images"
    files=os.listdir(img_dir)
    for file in files:
        img_path=img_dir+"/"+file
        anno_path=Anno_dir+"/"+file[:-3]+"xml"
        if not os.path.exists(anno_path):
            noanoimg_path=noannodir+"/"+file
            shutil.move(img_path,noanoimg_path)
#滤除没有图片的标注
def filtererrorinannotations(args):
    rootdir=args.dataset_dir
    Anno_dir=rootdir+"/"+"Annotations"
    img_dir=rootdir+"/"+"images"
    files=os.listdir(Anno_dir)
    for file in files:
        anno_path=Anno_dir+"/"+file
        img_path=img_dir+"/"+file[:-3]+image_ext
        if not os.path.exists(img_path):
            noano_path=noannodir+"/"+file
            shutil.move(anno_path,noano_path)
#滤除数据集中的错误
def filtererrors(args):
    if not os.path.exists(noannoimgdir):
        os.makedirs(noannoimgdir)
    if not os.path.exists(noannodir):
        os.makedirs(noannodir)
    filtererrorsinimages(args)
    filtererrorinannotations(args)
def get_images_hasannotations(args):
    files=os.listdir(annodir)
    dst_dir=datasetdir+"/images"
    if not os.path.exists(dst_dir):
        os.mkdir(dst_dir)
    for file in files:
        filepath=annodir+"/"+file
        if filepath.endswith("xml"):
            print(filepath)
            image_path=imagesdir+"/"+file[:-3]+"png"
            dst_path=dst_dir+"/"+file[:-3]+"png"
            shutil.copy(image_path,dst_path)
#get all annotation labels and distribution
def get_all_labels(args):
    dataset_dir=args.dataset_dir
    Annotation_dir=dataset_dir+"/Annotations"
    images_dir=dataset_dir+"/"+images_dir_rel
    gt_dir=dataset_dir+"/gt"
    mkr(images_dir)
    mkr(gt_dir)
    files=os.listdir(Annotation_dir)
    labeldistributions={}
    total=0
    #for file in files:
    for file in tqdm(files):
        annopath=Annotation_dir+"/"+file
        annoxml=ET.parse(annopath) 
        objects=annoxml.findall('object')
        imgfilename=file[:-3]+image_ext
        if args.show_annotations:
            imagepath=images_dir+"/"+imgfilename
            image=cv2.imread(imagepath)
            if image is None:
                continue
        for i in range(len(objects)):
            total=total+1
            name=objects[i].find("name").text
            if name in labeldistributions:
                labeldistributions[name]=labeldistributions[name]+1
            else:
                labeldistributions[name]=1
            if args.show_annotations or args.save_gt or args.save_classified:
                bndbox=objects[i].find("bndbox")
                rect=[]
                for i in range(len(bndbox)):
                    rect.append(float(bndbox[i].text))  
                cv2.rectangle(image,(int(rect[0]),int(rect[1])),(int(rect[2]),int(rect[3])),(255,0,0))
                cv2.putText(image,name,(int(rect[0]),int(rect[1])),3,1,(0,255,0))
                if args.save_classified:
                    save_class_dir=gt_dir+"/"+name
                    mkr(save_class_dir)
                    gt_path=save_class_dir+"/"+imgfilename
                else:
                    gt_path=gt_dir+imgfilename
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
#split train and test for training
def split_traintest(args,trainratio=0.8):
    dataset_dir=args.dataset_dir
    with open(dataset_dir+"/trainval.txt","w")as ftrain:
        with open(dataset_dir+"/test.txt","w") as ftest:
            annodir=dataset_dir+"/Annotations"
            files=os.listdir(annodir)
            for i in range(len(files)):
                filepath=dataset_dir+"/images/"+files[i][:-3]+"png"
                if i<trainratio*len(files):
                    ftrain.write(filepath+"\n")
                else:
                    ftest.write(filepath+"\n")

def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--dataset_dir', type=str,help='Path to the data directory of dataset.',
                        default=
                        #"D:/Detection/Datasets/voc/VOCdevkit/VOC2007"
                        "D:/Detection/MRLabeler/bin/CK2018"
                        )
    parser.add_argument('--show_annotations', type=bool,help='show annotations on label analysis',default=True)
    parser.add_argument('--save_gt', type=bool,help='classified show gt',default=True)
    parser.add_argument('--save_classified', type=bool,help='classified show gt',default=True)
    parser.add_argument('--extention', type=str,help='dataset image file extention',default=".png")
    return parser.parse_args()

def main():
    args=get_args()
    filtererrors(args)
    #split_traintest(args)
    get_all_labels(args)

if __name__=="__main__":
    main()