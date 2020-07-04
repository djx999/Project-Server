#include<TcpKernel.h>
#include<TCPNet.h>
#include<string.h>
#include<unistd.h>

	BEGIN_PROTOCOL_MAP
	PM(_DEF_PROTOCOL_REGISTER_RQ,&RegisterRq)
	PM(_DEF_PROTOCOL_LOGIN_RQ,&LoginRq)
	PM(_DEF_PROTOCOL_VEDIO,&SaveVideoInfo)
	PM(_DEF_PROTOCOL_VEDIO_RQ,&SaveVideoContent)
	PM(_DEF_PROTOCOL_WATCKVIDEO_RQ,&WatchVideo)
	PM(_DEF_PROTOCOL_ADDLOVENUM,&AddLoveNum)
	PM(_DEF_PROTOCOL_DOWNLOADINFO,&DownLoadInfo)
PM(_DEF_PROTOCOL_DOWNLOADCONTENT,&SendloadContent)
	END_PROTOCOL_MAP

int Kernel_open()
{
	InitMySql();
	L_Init(&list);
	L_Init(&downlist);
	ConnectMySql("localhost","root","root","douyin");
	EpollNet();

}
void Kernel_close()
{
	UnInitNet();
	DisConnect();
}
void BubbleSort(double arr[],int nlen,int arrvid[])
{
	for(int i=0;i<nlen-1;i++)
	{
		for(int j=0;j<nlen-1-i;j++)
		{
			if(arr[j]>arr[j+1])
			{
				printf("----\n");
				double temp = arr[j];
				arr[j] = arr[j+1];
				arr[j+1] = temp;
				arrvid[j] = arrvid[j]^arrvid[j+1];
				arrvid[j+1] = arrvid[j]^arrvid[j+1];
				arrvid[j] = arrvid[j]^arrvid[j+1];

			}
		}
	}

}
void Probs()
{
	//1.查看数据库表中行列的数 行：用户  列：视频
	int row,col;
	char szsql[1024]={0};
	List *list;
	L_Init(&list);
	sprintf(szsql,"select count(*) from data");
	SelectMySql(szsql,1,list);
	char *nrow = (char*)L_Front(list);
	L_Pop(list);
	row = atoi(nrow);
	printf("原数据表中行数为:%d\n",row);
	sprintf(szsql,"select count(*)from video");
	SelectMySql(szsql,1,list);
	char *ncol = (char*)L_Front(list);
	L_Pop(list);
	col = atoi(ncol);
	printf("原数据表中列数为:%d\n",col);
	int arr[row][col]; //二维数组
	int arr1[row][col];
	//查看当前行所以数据
	for(int i=1;i<=row;i++)
	{
		sprintf(szsql,"select *from data where id=%d",i);
		SelectMySql(szsql,col+2,list);
		L_Pop(list);
		L_Pop(list);
		for(int j=0;j<col;j++)
		{
			char *vid1 = (char*)L_Front(list);
			L_Pop(list);
			int id = atoi(vid1);
			if(id == 2)
			{
				arr[i-1][j] = 1;
				arr1[i-1][j] = 2;
			}
			else if(id == 1)
			{
				arr[i-1][j] = 0;
				arr1[i-1][j] = 1;
			}
			else
			{
				arr[i-1][j] = 0;
				arr1[i-1][j] = 0;
			}

		}
	}
	for(int i=0;i<row;i++)
	{
		for(int j=0;j<col;j++)
		{
			printf("%d",arr[i][j]);
		}
		printf("\n");
	}

	//第二步
	double weights[col][col];
	for(int i=0;i<col;i++)
	{
		for(int j=0;j<col;j++)
		{			
			int count = 0;
			for(int t=0;t<row;t++)
			{
				if(arr[t][j] == 1)
					count++;
			}
			double degree = (double)count;
			double sum = 0.00; //视频收到所以用户的资源
			//遍历用户
			double source;
			double degree1;
			for(int k=0;k<row;k++)
			{
				source = (double)(arr[k][i]*arr[k][j]);
				int ncount = 0;
				for(int h=0;h<col;h++)
				{
					if(arr[k][h] == 1)
						ncount++;
				}
				if(ncount!=0)
				{
					degree1 = (double)ncount;
					sum+=source/degree1;
				}
			}
			if(degree)
				weights[i][j] = sum/degree;
		}

	}
	for(int i=0;i<col;i++)
	{
		for(int j=0;j<col;j++)
		{
			printf("%f ",weights[i][j]);
		}
		printf("\n");
	}

	//第三步  给权重数组赋值
	double score[row][col];
	double res = 0;
	for(int i=0;i<row;i++)
	{
		for(int j=0;j<col;j++)
		{
			res = 0;
			if(arr1[i][j] == 0)
			{
				//计算内积
				for(int t=0;t<col;t++)
				{
					res+=weights[j][t]*arr[i][t];
				}
				score[i][j] = res;
			}
			char name[20];
			strcpy(name,"vid");
			char buf[20];
			sprintf(buf,"%d",j+1);
			char tempname[100];
			strcat(name,buf);
			strcpy(tempname,name);
			//printf("%s\n",tempname); //将名字拼接
			sprintf(szsql,"update weights set %s=%f where id = %d",tempname,res,i+1);
			printf("%s\n",szsql);
			UpdateMySql(szsql);
		}
	}
	printf("\n");
	for(int i=0;i<row;i++)
	{
		for(int j=0;j<col;j++)
		{
			printf("%f ",score[i][j]);

		}
		printf("\n");
	}

}
//服务器发送视频内容
void SendloadContent(unsigned int clientfd,char* szbuf)
{
	printf("发送内容协议..\n");
	struct STRU_FILECONTENT *sfl = (struct STRU_FILECONTENT*)szbuf;
	//遍历用户下载链表  找到对应的用户
	printf("下载用户链表值为:%d\n",downlist->nCount);
	List *pTemp = downlist;
	struct UserDownload_Info *pInfo = NULL;
	while(pTemp)
	{
		pInfo = (struct UserDownload_Info*)pTemp->pHead->nValue;
		if(pInfo->u_id == sfl->m_uid)
		{
			printf("下载链表用户id为:%d\n",pInfo->u_id);
			break;
		}
		pTemp->pHead = pTemp->pHead->pNext;
	}
	printf("查询下载用户成功..\n");
	if(pInfo->m_nSize == pInfo->m_nPos)
	{
		//证明用户下载完毕  将用户移除链表
		List *pNode = downlist;
		struct UserDownload_Info *p = NULL;
		while(pNode)
		{
			p = (struct UserDownload_Info*)pNode->pHead->nValue;
			if(p->u_id == sfl->m_uid)
			{
				if(downlist->nCount >0)
				{
					if(downlist->nCount==1)
					{
						downlist->nCount--;
						downlist->pHead = NULL;
						printf("删除成功..\n");
						printf("%d\n",downlist->nCount);
						return;
					}
					else
					{
						downlist->pHead = downlist->pHead->pNext;
						return;
					}
				}
			}
			pNode->pHead = pNode->pHead->pNext;
		}
	}
	if(pInfo->m_nPos == 500000)
		sleep(10);

	//发送视频内容
	struct STRU_CONTENT scu;
	scu.m_nType = _DEF_PROTOCOL_CONTENT;
	pInfo->m_pFile = fopen(pInfo->m_szpath,"rb");
	fseek(pInfo->m_pFile,pInfo->m_nPos,SEEK_SET);
	scu.m_nSize = fread(scu.m_nContent,sizeof(char),sizeof(scu.m_nContent),pInfo->m_pFile);
	pInfo->m_nPos += scu.m_nSize;
	fclose(pInfo->m_pFile);
	printf("第一次发送的大小为:%d\n",scu.m_nSize);
	printf("偏移量:%d\n",pInfo->m_nPos);
	SendData(clientfd,(char*)&scu,sizeof(scu));

}
void DownLoadInfo(unsigned int clientfd,char* szbuf)
{
	struct STRU_LoadRq *sdl = (struct STRU_LoadRq*)szbuf;
	if(sdl->m_Flag ==1) //判断标记  断点续传
	{
		//查找下载用户链表 找到偏移量
		List *pNode = downlist;
		struct UserDownload_Info *pInfo = NULL;
		while(pNode)
		{
			pInfo = (struct UserDownload_Info*)pNode->pHead->nValue;
			if(pInfo->u_id == sdl->m_uid)
			{
				pInfo->m_nPos = sdl->m_loadPos;
				break;
			}
			pNode->pHead = pNode->pHead->pNext;
		}
		if(pInfo->m_nSize == pInfo->m_nPos)
		{

			//证明用户下载完毕  将用户移除链表
			List *pNode = downlist;
			struct UserDownload_Info *p = NULL;
			while(pNode)
			{
				p = (struct UserDownload_Info*)pNode->pHead->nValue;
				if(p->u_id == sdl->m_uid)
				{
					if(downlist->nCount >0)
					{
						if(downlist->nCount==1)
						{
							downlist->nCount--;
							downlist->pHead = NULL;
							printf("删除成功..\n");
							printf("%d\n",downlist->nCount);
							return;
						}
						else
						{
							downlist->pHead = downlist->pHead->pNext;
							return;
						}
					}
				}
				pNode->pHead = pNode->pHead->pNext;
			}

		}

		//断点续传发送内容
		struct STRU_CONTENT scu;
		scu.m_nType = _DEF_PROTOCOL_CONTENT;
		pInfo->m_pFile = fopen(pInfo->m_szpath,"rb");
		fseek(pInfo->m_pFile,pInfo->m_nPos,SEEK_SET);
		scu.m_nSize = fread(scu.m_nContent,sizeof(char),sizeof(scu.m_nContent),pInfo->m_pFile);
		pInfo->m_nPos += scu.m_nSize;
		fclose(pInfo->m_pFile);
		printf("第一次发送的大小为:%d\n",scu.m_nSize);
		printf("偏移量:%d\n",pInfo->m_nPos);
		SendData(clientfd,(char*)&scu,sizeof(scu));

	}

	else
	{
		//将用户信息加入到链表中
		char szsql[1024]={0};
		List *lststr; //sql链表
		L_Init(&lststr); //sql链表
		struct UserDownload_Info *pInfo = (struct UserDownload_Info*)malloc(sizeof(struct UserDownload_Info));
		pInfo->m_bflag1 = 1;
		pInfo->m_vid = sdl->m_vid;
		pInfo->m_nPos = 0;
		pInfo->u_id = sdl->m_uid;
		pInfo->m_nSize = 0;
		pInfo->m_pFile = NULL;
		sprintf(szsql,"select v_name,v_path from video where v_id=%d",pInfo->m_vid);
		if(SelectMySql(szsql,2,lststr)==0)
		{
			char* name = (char*)L_Front(lststr);
			L_Pop(lststr);
			printf("下载查询的name:%s\n",name);
			char* path = (char*)L_Front(lststr);
			L_Pop(lststr);
			strcpy(pInfo->m_szpath,path);
			printf("下载查询的路径:%s\n",pInfo->m_szpath);
			pInfo->m_pFile = fopen(path,"rb");
			fseek(pInfo->m_pFile,0,SEEK_END);
			int nFileSize = ftell(pInfo->m_pFile);
			fseek(pInfo->m_pFile,0,SEEK_END);
			pInfo->m_nSize = nFileSize;
			printf("发送下载文件大小为:%d\n",pInfo->m_nSize);
			L_Push(downlist,(void*)pInfo);
			fclose(pInfo->m_pFile);
			//将文件大小发送
			struct STRU_FILESIZE sss;
			sss.m_nType = _DEF_PROTOCOL_DOWNLOADSIZE;
			sss.m_nFileSize = nFileSize;
			strcpy(sss.m_nFileName,name);
			SendData(clientfd,(char*)&sss,sizeof(sss));

		}

	}
}
//点赞函数
void AddLoveNum(unsigned int clientfd,char* szbuf)
{
	struct STRU_LOVENUM *sl = (struct STRU_LOVENUM*)szbuf;
	char szsql[1024]={0};
	sprintf(szsql,"update douyin.video set v_num = %d where v_id =%d",sl->m_videoLove,sl->m_videoId);
	printf("%s\n",szsql);
	if(UpdateMySql(szsql)==0)
	{
		printf("点赞数更新成功..\n");
		char nameaa[20];
		strcpy(nameaa,"vid");
		char buf[20];
		sprintf(buf,"%d",sl->m_videoId);
		char tempname[100];
		strcat(nameaa,buf);
		strcpy(tempname,nameaa);
		//printf("%s\n",tempname); //将名字拼接
		sprintf(szsql,"update data set %s=%d where id = %d",tempname,2,sl->m_userid);
		printf("%s\n",szsql);
		UpdateMySql(szsql);
	}

}

//接收客户端观看视频请求
void WatchVideo(unsigned int clientfd,char* szbuf)
{
	struct STRU_WATCHVIDEO_RQ *swr = (struct STRU_WATCHVIDEO_RQ*)szbuf;
	Probs();

	//1.查找用户id找出权重表中对应的全部信息
	char szsql[1024] = {0};
	List *list;
	int count1 = 0;
	int n;
	L_Init(&list);
	sprintf(szsql,"select count(*) from video");
	SelectMySql(szsql,1,list);
	char *ncount = (char*)L_Front(list);
	L_Pop(list);
	count1 = atoi(ncount); //count1视频个数
	//解决冷启动问题  
	sprintf(szsql,"select *from data where u_id=%d",swr->m_userid);
	SelectMySql(szsql,count1+2,list);
	L_Pop(list);
	L_Pop(list);
	int dataarr[count1];
	int nFlag = 0;
	for(int i=0;i<count1;i++)
	{
		char *datanum = (char*)L_Front(list);
		L_Pop(list);
		dataarr[i] = atoi(datanum);

	}
	for(int i=0;i<count1;i++)
	{
		printf("%d ",dataarr[i]);
	}
	for(int i=0;i<count1;i++)
	{
		if(dataarr[i] == 0)
		{
			nFlag = 0;	//0代表新用户
			continue;
		}
		else
		{
			nFlag = 1;  //1代表老用户
		}
	}
	if(nFlag == 1) //新用户
	{
		double arr[count1];
		bzero(&szsql,sizeof(szsql));
		sprintf(szsql,"select *from weights where u_id = %d",swr->m_userid);
		printf("%s\n",szsql);
		SelectMySql(szsql,count1+2,list);
		L_Pop(list);
		L_Pop(list);
		for(int i=0;i<count1;i++)
		{
			char *num = (char*)L_Front(list);
			L_Pop(list);
			arr[i] = atof(num);
		}
		//2.将其进行排序 
		int arrvid[count1];
		for(int i=0;i<count1;i++)
		{
			arrvid[i] = i+1;
		}
		BubbleSort(arr,count1,arrvid);
		for(int i=0;i<count1;i++)
		{
			printf("%f ",arr[i]);
		}
		for(int i=0;i<count1;i++)
		{
			printf("%d ",arrvid[i]);
		}
		//3.找到权重最大的视频对应的路径 如果权重值相等 按顺序推 
		int i;  //n是要推送的视频id
		for( i=count1-1;i>=0;i--)
		{
			if(arr[i]!=0.000000)
			{
				n = arrvid[i];
				break;
			}
		}	
	}
	else
	{

		n = count1;
	}

	//将视频推送给客户端
	struct STRU_UPLOAD_RQ sur;
	sur.m_nType = _DEF_PROTOCOL_UPLOADVIDEO_RQ;
	//char szsql[1024]={0};
	List *lststr;
	L_Init(&lststr);
	char* path;
	char  name[1024];
	char* type;
	char *title;
	int   num;
	sprintf(szsql,"select v_path,v_name,v_type,v_title,v_num from douyin.video where v_id = %d",n);
	printf("%s\n",szsql); 
	if(SelectMySql(szsql,5,lststr)==0)
	{
		int n1 = L_Isempty(lststr);
		printf("%d\n",n1);
		path = (char*)L_Front(lststr);
		L_Pop(lststr);
		strcpy(name,(char*)L_Front(lststr));
		L_Pop(lststr);
		type = (char*)L_Front(lststr);
		L_Pop(lststr);
		title = (char*)L_Front(lststr);
		L_Pop(lststr);
		char* n = (char*)L_Front(lststr);
		L_Pop(lststr);
		num = atoi(n);
	}
	char Sql[1024]={0};
	List *lststr1;
	L_Init(&lststr1);
	FILE *pFile;
	int count;
	sprintf(Sql,"select count(*) from video");
	printf("%s\n",Sql);
	if(SelectMySql(Sql,1,lststr1)==0)
	{
		char* ncount = (char*)L_Front(lststr1);
		L_Pop(lststr1);
		count = atoi(ncount);
	}
	//打开指定路径文件
	pFile = fopen(path,"rb");
	fseek(pFile,0,SEEK_END);
	int nFileSize = ftell(pFile);
	printf("%d\n",nFileSize);
	sur.m_VideoSize = nFileSize;
	fseek(pFile,0,SEEK_SET);
	strcpy(sur.m_VideoName,name);
	printf("%s\n",sur.m_VideoName);
	SendData(clientfd,(char*)&sur,sizeof(sur));

	STRU_UPLOADINFO_RQ slr;
	slr.m_nType = _DEF_PROTOCOL_UPLOADVIDEOINFO;
	strcpy(slr.m_VideoName,name);
	strcpy(slr.m_VideoType,type);
	strcpy(slr.m_VideoTitle,title);
	slr.m_VideoLove = num;
	slr.m_VideoId = n;
	slr.m_VideoCount = count;
	while(1)
	{
		slr.m_VideoSize =fread(slr.m_VideoContent,sizeof(char),sizeof(slr.m_VideoContent),pFile);
		if(slr.m_VideoSize == 0)
		{
			fclose(pFile);
			//修改data表中推给用户视频由0->1
			char nameaa[20];
			strcpy(nameaa,"vid");
			char buf[20];
			sprintf(buf,"%d",n);
			char tempname[100];
			strcat(nameaa,buf);
			strcpy(tempname,nameaa);
			//printf("%s\n",tempname); //将名字拼接
			sprintf(szsql,"update data set %s=%d where id = %d",tempname,1,swr->m_userid);
			printf("%s\n",szsql);
			UpdateMySql(szsql);
			return;
		}
		SendData(clientfd,(char*)&slr,sizeof(slr));
	}



}

//接收客户端传过来的视频  判断视频相关信息进行优化
void SaveVideoContent(unsigned int clientfd,char* szbuf)
{
	struct STRU_VEDIOINFO_RQ *svr = (struct STRU_VEDIOINFO_RQ*)szbuf;
	List *pTemp = list;
	struct Author_Info *pNode = NULL;
	while(pTemp)
	{
		pNode = (struct Author_Info*)pTemp->pHead->nValue;
		if(pNode->sock == clientfd)
			break;
		pTemp->pHead = pTemp->pHead->pNext;
	}
	int nRealReadNum = 0;
	char Filename[1024]={0};
	char szsql[1024]={0};
	List *SearchList;
	L_Init(&SearchList);
	strcpy(Filename,pNode->pInfoFileName);
	//	printf("接收内容时名字为:%s\n",Filename);
	nRealReadNum = fwrite(svr->m_szContent,sizeof(char),svr->m_nVedioLen,pNode->pFile);
	//printf("%d\n",nRealReadNum);
	pNode->offset+=nRealReadNum;
	//printf("%d\n",pNode->nSize);
	//printf("%d\n",pNode->offset);
	if(pNode->offset == pNode->nSize)
	{
		//	printf("进来.\n");
		fclose(pNode->pFile);
		pNode->pFile = NULL;
		//	pNode->offset=0;
		//	pNode->nSize=0;
		//先从数据库查询视频总数
		sprintf(szsql,"select count(*) from video");
		//printf("%s\n",szsql);
		SelectMySql(szsql,1,SearchList);
		char* count = (char*)L_Front(SearchList);
		L_Pop(SearchList);
		int nCountVideo = atoi(count);
		printf("查询数据库总条数为:%d\n",nCountVideo);
		for(int i=1;i<=nCountVideo;i++)
		{
			sprintf(szsql,"select u_id from video where v_id=%d",i);
			SelectMySql(szsql,1,SearchList);
			char* m = (char*)L_Front(SearchList);
			L_Pop(SearchList);
			int m_userid = atoi(m);
			printf("查询到的用户id:%d\n",m_userid);
			if(m_userid == svr->u_id)
			{
				printf("查询用户id判断成功\n");
				sprintf(szsql,"select v_name,v_path,v_type,v_title from video where v_id = %d\n",i);
				SelectMySql(szsql,4,SearchList);
				char* SearchName = (char*)L_Front(SearchList);
				L_Pop(SearchList);
				printf("查询到的名字为:%s\n",SearchName);
				char* SearchPath = (char*)L_Front(SearchList);
				L_Pop(SearchList);
				printf("查询到的路径为:%s\n",SearchPath);
				char* SearchType = (char*)L_Front(SearchList);
				L_Pop(SearchList);
				printf("查询到的类型为:%s\n",SearchType);
				char* SearchTitle = (char*)L_Front(SearchList);
				L_Pop(SearchList);
				printf("查询到的标题:%s\n",SearchTitle);
				printf("包里的标题为:%s\n",svr->m_szTitle);
				if(strcmp(SearchTitle,svr->m_szTitle)==0)
				{
					printf("标题相同：\n");
					//计算数据库对应相同标题的视频大小
					FILE *p = NULL;
					p = fopen(SearchPath,"rb");
					fseek(p,0,SEEK_END);
					int SearchSize = ftell(p);
					fseek(p,0,SEEK_SET);
					char szname[100];
					strcpy(szname,pNode->pInfoFileName);
					char Uppath[100] = "./tempfile/";
					strcat(Uppath,szname);
					printf("打开拼接的路径:%s\n",Uppath);
					pNode->pFile = fopen(Uppath,"rb");
					if(pNode->nSize == SearchSize)
					{
						printf("大小相等进来..\n");
						//比较内容
						char buffer1[1000] = {0};
						char buffer2[1000] = {0};
						int readbyte1 = 0;
						int readbyte2 = 0;
						while(1)
						{
							readbyte1 = fread(buffer1,sizeof(char),sizeof(buffer1),p); //数据库的
							readbyte2 = fread(buffer2,sizeof(char),sizeof(buffer2),pNode->pFile); //用户上传的
							if(readbyte2>0&&readbyte1>0)
							{
								int nlen = memcmp(buffer2,buffer1,1000);
								if(nlen == 0)
								{
									printf("neirongxiangdeng\n");
									continue;
								}
								else
								{
									printf("内容不桶\n");
									fclose(pNode->pFile);
									pNode->pFile = NULL;
									fclose(p);
									p = NULL;
								}

							}
							else
							{
								printf("完全相同..\n");
								fclose(pNode->pFile);
								pNode->pFile = NULL;
								fclose(p);
								p = NULL;

								return;
							}

						}
					}

				}

			}

		} 
		printf("不完全相同:\n");
		//将视频信息插入到数据库中
		sprintf(szsql,"insert into video(v_name,v_path,v_type,v_title,v_num,u_id) values('%s','%s','%s','%s',%d,%d)",
				pNode->pInfoFileName,szpath,svr->m_szType,svr->m_szTitle,svr->m_nLove,svr->u_id);
		printf("%s\n",szsql);
		int n = UpdateMySql(szsql);
		if(n == 0)
		{
			printf("视频信息插入成功.\n");
		}
		else
		{
			printf("视频信息插入失败.\n");
		}
		//将视频插入到data表中  作为列
		int videocount;
		List *videoList;
		L_Init(&videoList);
		sprintf(szsql,"select count(*) from video");
		SelectMySql(szsql,1,videoList);
		char *idnum = (char*)L_Front(videoList);
		L_Pop(videoList);
		videocount = atoi(idnum);
		char name[20];
		strcpy(name,"vid");
		char buf[20];
		sprintf(buf,"%d",videocount);
		char tempname[100];
		strcat(name,buf);
		strcpy(tempname,name);
		//printf("%s\n",tempname); //将名字拼接
		sprintf(szsql,"alter table weights add column %s double not null default 0",tempname);
		printf("%s\n",szsql);
		UpdateMySql(szsql);
		sprintf(szsql,"alter table data add column %s int not null default 0",tempname);
		printf("%s\n",szsql);
		UpdateMySql(szsql);
		return;

	}

}
//接收客户端发送过来的视频信息
void SaveVideoInfo(unsigned int clientfd,char* szbuf)
{
	STRU_VEDIOINFO *svr = ( STRU_VEDIOINFO*)szbuf;
	//遍历用户链表  找到对应用户的相关信息
	List *pTemp = list;
	struct Author_Info *pNode = NULL;
	while(pTemp)
	{
		pNode = (struct Author_Info*)pTemp->pHead->nValue;
		if(pNode->sock == clientfd)
			break;
		pTemp->pHead = pTemp->pHead->pNext;
	}
	//查询数据库总数 判断名字是否相同
	List *UpList;
	L_Init(&UpList);
	char szsql[1024]={0};
	sprintf(szsql,"select count(*) from video");
	SelectMySql(szsql,1,UpList);
	char *p = (char*)L_Front(UpList);
	L_Pop(UpList);
	int nCount = atoi(p);
	char UpName[1024];
	for(int i=1;i<=nCount;i++)
	{
		sprintf(szsql,"select v_name from video where v_id=%d",i);
		SelectMySql(szsql,1,UpList);
		strcpy(UpName,(char*)L_Front(UpList));
		L_Pop(UpList);
		printf("查询到名字为:%s\n",UpName);
		//视频名字相同
		if(strcmp(UpName,svr->m_szFilename)==0)
		{
			strcat(svr->m_szFilename,".mp4");
			char StrcatName[1024]={0};
			strcpy(StrcatName,svr->m_szFilename);
			printf("拼接后的文件名为:%s\n",StrcatName);
			pNode->nSize = svr->m_nVedioLen;
			printf("拼接收到的大小为:%d\n",pNode->nSize);
			strcpy(szpath,"./tempfile/");
			strcat(szpath,StrcatName);
			printf("拼接接收文件路径为:%s\n",szpath);
			strcpy(pNode->pInfoFileName,StrcatName);
			printf("付给用户拼接后的名:%s\n",pNode->pInfoFileName);
			pNode->pFile = fopen(szpath,"wb");

		}
		else
		{

			//	printf("大小为:%d\n",svr->m_nVedioLen);
			pNode->nSize = svr->m_nVedioLen;
			//	printf("收到的大小为:%d\n",pNode->nSize);
			char Filename[1024] = {0};
			strcpy(Filename,svr->m_szFilename);
			//	printf("收到的文件名:%s\n",Filename);
			strcpy(szpath,"./tempfile/");
			strcat(szpath,svr->m_szFilename);
			//	printf("接收文件路径为:%s\n",szpath);
			strcpy(pNode->pInfoFileName,Filename);
			pNode->pFile = fopen(szpath,"wb");
		}
	}




}
void LoginRq(unsigned int clientfd,char* szbuf)
{
	STRU_LOGIN_RQ *slr = ( STRU_LOGIN_RQ*)szbuf;
	STRU_LOGIN_RS pslr;
	pslr.m_nType = _DEF_PROTOCOL_LOGIN_RS;
	pslr.m_szResult = login_fail;
	char szsql[1024] = {0};
	List *lststr;
	L_Init(&lststr);
	char password[45];
	sprintf(szsql,"select u_password from usertable where u_id = %d or u_name = '%s'",slr->m_userid,slr->m_szName);
	if(SelectMySql(szsql,5,lststr)==0)
	{
		printf("%s\n",szsql);
		if(L_Isempty(lststr)==1)
		{
			printf("---\n");
			printf("链表为空\n");
		}
		else
		{
			//printf("----\n");
			strcpy(password,(char*)L_Front(lststr));
			L_Pop(lststr);
			printf("查询数据库密码为:%s\n",password);
			if(strcmp(password,slr->m_szPassword)==0)
			{
				printf("登录成功..\n");
				pslr.m_szResult = login_success;
				//将用户信息加入到链表中
				struct Author_Info *pInfo = (struct Author_Info*)malloc(sizeof(struct Author_Info));
				pInfo->sock = clientfd;
				pInfo->m_id = 1;
				pInfo->pFile = NULL;
				pInfo->offset = 0;
				pInfo->nSize =0 ;
				L_Push(list,(void*)pInfo);
			}
			else
			{
				pslr.m_szResult = login_fail;
			}
			SendData(clientfd,(char*)&pslr,sizeof(pslr));
		}
	}
	else
	{
		printf("查询失败\n");
	}


}
void RegisterRq(unsigned int clientfd,char* szbuf)
{
	printf("--------\n");
	struct STRU_REGISTER_RQ *srr = (struct STRU_REGISTER_RQ*)szbuf;
	struct STRU_REGISTER_RS psrr;
	psrr.m_nType = _DEF_PROTOCOL_REGISTER_RS;
	psrr.m_szResult = register_fail;
	//printf("%d\n",srr->m_userid);
	//printf("%s\n",srr->m_szName);
	//printf("%s\n",srr->m_szPassword);
	char szsql[1024];
	bzero(&szsql,sizeof(szsql));
	sprintf(szsql,"insert into usertable values(%d,'%s','%s')",srr->m_userid,srr->m_szName,srr->m_szPassword);
	printf("%s\n",szsql);
	int n = UpdateMySql(szsql);
	if(n == 0)
	{
		printf("注册成功..\n");
		psrr.m_szResult = register_success;
	}
	else
	{
		printf("注册失败..\n");
		psrr.m_szResult = register_fail;
	}
	sprintf(szsql,"insert into data(u_id) values(%d)",srr->m_userid);
	UpdateMySql(szsql);
	SendData(clientfd,(char*)&psrr,sizeof(psrr));
	sprintf(szsql,"insert into weights(u_id) values(%d)",srr->m_userid);
	UpdateMySql(szsql);
	SendData(clientfd,(char*)&psrr,sizeof(psrr));
	printf("回复成功..\n");
}


int DealData(unsigned int clientfd,char* szbuf)
{
	//printf("----\n");
	PackType *pType = (PackType*)szbuf;
	//	printf("%d\n",*pType);
	int i=0;
	while(1)
	{
		if(m_ProtocolMapEntries[i].m_nType == *pType)
		{
			(*m_ProtocolMapEntries[i].m_npfun)(clientfd,szbuf);
			//printf("------\n");
			break;
		}
		else if((m_ProtocolMapEntries[i].m_nType == 0) &&( m_ProtocolMapEntries[i].m_npfun ==0))
		{
			//printf("----\n");
		}

		i++;
	}
	return 0;	
}
