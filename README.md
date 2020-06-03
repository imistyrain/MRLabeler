# MRLabeler

#### VOC YOLO 数据集标注工具 V1.4

![](https://i.imgur.com/5YDypuW.png)

Change log:

1.4 添加tooltip，更改添加标注框为按shift键以提升标注速度

1.3 添加由Video自动生成标注工程

1.2 添加帮助文件支持

1.1 添加列表框支持鼠标选择文件进行标注，添加键盘切换图片功能

1.0 初版发布，支持矩形框拖动功能

## 快速上手

参考[MRLabeler使用说明.pdf](MRLabeler使用说明.pdf)

## 编译方法

* 1.按照[MRHead](https://github.com/imistyrain/MRHead)的方法搭建好跨平台OpenCV编译环境

* 2.用VS2013打开MRLabeler.sln编译即可

本项目严格按照VOC和YOLO方式组织，各文件夹统一放置到一个目录下，记为DATASETDIR，其中images文件夹用于存放原始图片，Annotations文件夹用于存放VOC格式的标注，labels用于存放YOLO格式的标注，mrconfig.xml作为DATASETDIR数据集的配置文件。

![](http://i.imgur.com/A9qkTlH.png)

本项目从加载要标注数据集的相关信息，并将原标注一并显示，通过鼠标选中并拖动框的位置，点击下一张(>按钮)或者上一张(<按钮)保存。

你也可以直接在编辑框输入要跳转的索引，直接跳到要标注的位置。

组合框用于设置标注的类别，每次画框前要先对这个进行设置。

数据集配置文件中各个字段的含义如下：

```

<dataset>
	<name>IBM</name>数据集名称，自己定义
	<year>0712</year>数据集年代，为支持VOC而用
	<imagedir>Image</imagedir>数据集图片文件夹路径，相对于rootdir的路径
	<annotationdir>Annotations</annotationdir>原标注文件夹路径，相对于rootdir路径
	<labelsdir>labels</labelsdir>YOLO格式标注文件夹路径，相对于本项目的路径
	<currentlabelingclass>car</currentlabelingclass>当前要标注的类别名称
	<lastlabeledindex>0</lastlabeledindex>最后标注的类别索引
	<bsavexml>1</bsavexml>是否保存VOC格式标注，默认保存
	<bsavetxt>1</bsavetxt>是否保存YOLO格式标注，默认保存
	<classes>所有的类别，每个类别独占一行
		<class>face</class>
		<class>mask</class>
	</classes>
</dataset>

```