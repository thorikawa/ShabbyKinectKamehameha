#include <XnOpenNI.h>
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>
#include "SceneDrawer.h"
#include "dprintf.h"
#include <mmsystem.h>
#include <cmath>

#pragma comment(lib,"winmm")

using namespace xn;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------
xn::Context g_Context;
xn::DepthGenerator g_DepthGenerator;
xn::UserGenerator g_UserGenerator;
xn::ImageGenerator g_ImageGenerator;
xn::DepthMetaData g_depthMD;
xn::ImageMetaData g_imageMD;

// ishoku start
#define MAX_DEPTH 10000

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------
float g_pDepthHist[MAX_DEPTH];
XnRGB24Pixel* g_pTexMap = NULL;
unsigned int g_nTexMapX = 0;
unsigned int g_nTexMapY = 0;
// ishoku end

XnBool g_bNeedPose = FALSE;
XnChar g_strPose[20] = "";
XnBool g_bPrintID = TRUE;
XnBool g_bPrintState = TRUE;

#include <GL/glut.h>

#define GL_WIN_SIZE_X 720
#define GL_WIN_SIZE_Y 480

XnBool g_bPause = false;
XnBool g_bRecord = false;

XnBool g_bQuit = false;

MCI_OPEN_PARMS g_mop_kame1;
MCI_OPEN_PARMS g_mop_kame2;

//---------------------------------------------------------------------------
// Code
//---------------------------------------------------------------------------

void CleanupExit()
{
	g_Context.Shutdown();

	exit (1);
}

// Callback: New user was detected
void XN_CALLBACK_TYPE User_NewUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
	printf("New User %d\n", nId);
	// New user found
	if (g_bNeedPose)
	{
		g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
	}
	else
	{
		g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
	}
}
// Callback: An existing user was lost
void XN_CALLBACK_TYPE User_LostUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
	printf("Lost user %d\n", nId);
}
// Callback: Detected a pose
void XN_CALLBACK_TYPE UserPose_PoseDetected(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie)
{
	printf("Pose %s detected for user %d\n", strPose, nId);
	g_UserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
	g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
}
// Callback: Started calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie)
{
	printf("Calibration started for user %d\n", nId);
}
// Callback: Finished calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationEnd(xn::SkeletonCapability& capability, XnUserID nId, XnBool bSuccess, void* pCookie)
{
	if (bSuccess)
	{
		// Calibration succeeded
		printf("Calibration complete, start tracking user %d\n", nId);
		g_UserGenerator.GetSkeletonCap().StartTracking(nId);
	}
	else
	{
		// Calibration failed
		printf("Calibration failed for user %d\n", nId);
		if (g_bNeedPose)
		{
			g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
		}
		else
		{
			g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
		}
	}
}


XnFloat Colors[][3] =
{
	{0,1,1},
	{0,0,1},
	{0,1,0},
	{1,1,0},
	{1,0,0},
	{1,.5,0},
	{.5,1,0},
	{0,.5,1},
	{.5,0,1},
	{1,1,.5},
	{1,1,1}
};
XnUInt32 nColors = 10;

static GLfloat red[] = { 0.8, 0.2, 0.2, 1.0 };
static GLfloat green[] = { 0.2, 0.8, 0.2, 1.0 };
static GLfloat blue[] = { 0.2, 0.2, 0.8, 1.0 };
static GLfloat yellow[] = { 0.8, 0.8, 0.2, 1.0 };

void glPrintString(void *font, char *str)
{
	int i,l = strlen(str);

	for(i=0; i<l; i++)
	{
		glutBitmapCharacter(font,*str++);
	}
}

#define PI 3.1415926536

void circle(float cx, float cy, float _r)
{
	float x,y,z,r;
	r = _r;

	int n = 120;//ï™äÑêî
	int i;

	//glBegin( GL_LINE_LOOP ); //â~é¸Çê¸ÇæÇØÇ≈ï\é¶
	glBegin( GL_POLYGON ); //Ç±Ç¡ÇøÇæÇ∆ÅAâ~ÇÃì‡ïîÇ‹Ç≈ìhÇËÇ¬Ç‘ÇπÇÈ
	for(i=0;i<n;i++){
		x = r*cos(2.0*PI*(float)i/(float)n) + cx;
		y = r*sin(2.0*PI*(float)i/(float)n) + cy;
		z = -1.0;
		glVertex3f( x, y, z );
	}
	glEnd();
}

void ki(float x, float y, float s) {
	float r=s;
	while(s>0) {
		if (s>r*0.8){
			glColor4f(0.8, 0.8, 0.2, 0.3);
		}else{
			glColor4f(1, 1, 1, 0.3);
		}
		circle(x, y, s);
		s-=5;
	}

}

inline float dist (XnPoint3D pt0, XnPoint3D pt1) {
	float reald = pow((pt0.X-pt1.X), 2)+pow((pt0.Y - pt1.Y),2) + pow((pt0.Z - pt1.Z), 2);
	reald = pow(reald, 0.5F);
	return reald;
}

inline float norm (float x, float y, float z) {
	float n = pow(x, 2)+pow(y,2) + pow(z, 2);
	n = pow(n, 0.5F);
	return n;
}

inline float norm (float x, float y) {
	float n = pow(x, 2)+pow(y,2);
	n = pow(n, 0.5F);
	return n;
}

/**
* pt0,pt1,pt2ÇÃê¨Ç∑äp(pt0Ç™íÜêS)
*/
inline float degree (XnPoint3D pt0, XnPoint3D pt1, XnPoint3D pt2) {
	float x1 = pt1.X - pt0.X;
	float y1 = pt1.Y - pt0.Y;
	float z1 = pt1.Z - pt0.Z;
	float x2 = pt2.X - pt0.X;
	float y2 = pt2.Y - pt0.Y;
	float z2 = pt2.Z - pt0.Z;

	float prod = x1*x2 + y1*y2 + z1*z2;

	float cosv = prod/((norm(x1,y1,z1)*norm(x2,y2,z2)));
	return acos(cosv);
}

/**
* éwíËÇ≥ÇÍÇΩì_Ç…â~Çï`Ç≠
*/
void drawPoint (XnPoint3D pt) {
	XnPoint3D pts[1] = {pt};
	XnPoint3D projectedPt[1];
	g_DepthGenerator.ConvertRealWorldToProjective(1, pts, projectedPt);
	circle(projectedPt[0].X, projectedPt[0].Y, 20);
}

void dump (const char* msg, XnSkeletonJointPosition pos) {
	XnPoint3D pt = pos.position;
	printf("%s:(%f, %f, %f) confidence=%f\n", msg, pt.X, pt.Y, pt.Z, pos.fConfidence);
}

void drawRect ( float r, float h, float cx, float cy, float x, float y)
{
	float n = norm(x,y);
	float ux = x/n;
	float uy = y/n;
	float lbx = cx+uy*r;
	float lby = cy-ux*r;

	float rbx = cx-uy*r;
	float rby = cy+ux*r;

	float ltx = lbx+ux*h;
	float lty = lby+uy*h;

	float rtx = rbx+ux*h;
	float rty = rby+uy*h;

	glBegin(GL_QUADS);
	{
		glVertex2f(rbx, rby);
		glVertex2f(lbx, lby);
		glVertex2f(ltx, lty);
		glVertex2f(rtx, rty);
	}
	glEnd();
}

void ha (float r, float h, float cx, float cy, float x, float y) {
	glColor4f(0.8, 0.8, 0.2, 0.3);
	drawRect(r, h, cx, cy, x, y);
	glColor4f(1, 1, 1, 0.3);
	drawRect(r*0.8, h, cx, cy, x, y);
}

// this function is called each frame
void glutDisplay (void)
{
	XnStatus rc = XN_STATUS_OK;

	// Read a new frame
	rc = g_Context.WaitAnyUpdateAll();
	if (rc != XN_STATUS_OK)
	{
		printf("Read failed: %s\n", xnGetStatusString(rc));
		return;
	}

	g_DepthGenerator.GetMetaData(g_depthMD);
	g_ImageGenerator.GetMetaData(g_imageMD);

	const XnDepthPixel* pDepth = g_depthMD.Data();
	const XnUInt8* pImage = g_imageMD.Data();

	unsigned int nImageScale = GL_WIN_SIZE_X / g_depthMD.FullXRes();

	// Copied from SimpleViewer
	// Clear the OpenGL buffers
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Setup the OpenGL viewpoint
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, GL_WIN_SIZE_X, GL_WIN_SIZE_Y, 0, -1000.0, 1000.0);

	// Calculate the accumulative histogram (the yellow display...)
	xnOSMemSet(g_pDepthHist, 0, MAX_DEPTH*sizeof(float));

	unsigned int nNumberOfPoints = 0;

	// printf("depth XRes=%f, YRes=%f\n", g_depthMD.XRes(), g_depthMD.YRes());

	for (XnUInt y = 0; y < g_depthMD.YRes(); ++y)
	{
		for (XnUInt x = 0; x < g_depthMD.XRes(); ++x, ++pDepth)
		{
			if (*pDepth != 0)
			{
				g_pDepthHist[*pDepth]++;
				nNumberOfPoints++;
			}
		}
	}
	for (int nIndex=1; nIndex<MAX_DEPTH; nIndex++)
	{
		g_pDepthHist[nIndex] += g_pDepthHist[nIndex-1];
	}

	// printf("number of points=%d", nNumberOfPoints);

	if (nNumberOfPoints)
	{
		for (int nIndex=1; nIndex<MAX_DEPTH; nIndex++)
		{
			g_pDepthHist[nIndex] = (unsigned int)(256 * (1.0f - (g_pDepthHist[nIndex] / nNumberOfPoints)));
		}
	}

	xnOSMemSet(g_pTexMap, 0, g_nTexMapX*g_nTexMapY*sizeof(XnRGB24Pixel));

	// check if we need to draw image frame to texture
	glEnable(GL_TEXTURE_2D);

	const XnRGB24Pixel* pImageRow = g_imageMD.RGB24Data();
	XnRGB24Pixel* pTexRow = g_pTexMap + g_imageMD.YOffset() * g_nTexMapX;

	for (XnUInt y = 0; y < g_imageMD.YRes(); ++y)
	{
		const XnRGB24Pixel* pImage = pImageRow;
		XnRGB24Pixel* pTex = pTexRow + g_imageMD.XOffset();

		for (XnUInt x = 0; x < g_imageMD.XRes(); ++x, ++pImage, ++pTex)
		{
			*pTex = *pImage;
		}

		pImageRow += g_imageMD.XRes();
		pTexRow += g_nTexMapX;
	}

	// Create the OpenGL texture map
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_nTexMapX, g_nTexMapY, 0, GL_RGB, GL_UNSIGNED_BYTE, g_pTexMap);

	// Display the OpenGL texture map
	glColor4f(1,1,1,1);

	glBegin(GL_QUADS);

	int nXRes = g_depthMD.FullXRes();
	int nYRes = g_depthMD.FullYRes();

	// upper left
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	// upper right
	glTexCoord2f((float)nXRes/(float)g_nTexMapX, 0);
	glVertex2f(GL_WIN_SIZE_X, 0);
	// bottom right
	glTexCoord2f((float)nXRes/(float)g_nTexMapX, (float)nYRes/(float)g_nTexMapY);
	glVertex2f(GL_WIN_SIZE_X, GL_WIN_SIZE_Y);
	// bottom left
	glTexCoord2f(0, (float)nYRes/(float)g_nTexMapY);
	glVertex2f(0, GL_WIN_SIZE_Y);

	glEnd();

	// display string and kamehameha
	glDisable(GL_TEXTURE_2D);

	//ki(300,300,150);
	//ha(150,1000,300,300,1,1);

	char strLabel[50] = "";
	XnUserID aUsers[15];
	XnUInt16 nUsers = 15;
	g_UserGenerator.GetUsers(aUsers, nUsers);
	g_UserGenerator.GetUsers(aUsers, nUsers);
	for (int i = 0; i < nUsers; ++i) {
		if (g_bPrintID) {
			XnPoint3D com;
			g_UserGenerator.GetCoM(aUsers[i], com);
			g_DepthGenerator.ConvertRealWorldToProjective(1, &com, &com);

			xnOSMemSet(strLabel, 0, sizeof(strLabel));
			if (!g_bPrintState) {
				// Tracking
				sprintf(strLabel, "%d", aUsers[i]);
			} else if (g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i])) {
				// Tracking
				sprintf(strLabel, "%d - Tracking", aUsers[i]);
			} else if (g_UserGenerator.GetSkeletonCap().IsCalibrating(aUsers[i])) {
				// Calibrating
				sprintf(strLabel, "%d - Calibrating...", aUsers[i]);
			} else {
				// Nothing
				sprintf(strLabel, "%d - Looking for pose", aUsers[i]);
			}


			glColor4f(1-Colors[i%nColors][0], 1-Colors[i%nColors][1], 1-Colors[i%nColors][2], 1);

			glRasterPos2i(com.X, com.Y);
			glPrintString(GLUT_BITMAP_HELVETICA_18, strLabel);
		}

		if (g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i])){
			// find the distance between the left hand and the right hand
			XnSkeletonJointPosition joint1, joint2, jointW, jointLs, jointRs, jointLe, jointRe, jointLw, jointRw;
			g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(aUsers[i], XN_SKEL_LEFT_HAND, joint1);
			g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(aUsers[i], XN_SKEL_RIGHT_HAND, joint2);
			// g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(aUsers[i], XN_SKEL_WAIST, jointW);

			if(joint1.fConfidence < 0.5 || joint2.fConfidence < 0.5){
				// draw the same as before
				printf("not reliable\n");
			}else{
				XnPoint3D pt[2];
				XnPoint3D projectedPt[2];
				pt[0] = joint1.position;
				pt[1] = joint2.position;

				// printf("Real:x=%f,y=%f, z=%f\n", pt[0].X, pt[0].Y, pt[0].Z);
				float reald = dist(pt[0], pt[1]);
				printf("reald=%f\n", reald);

				g_DepthGenerator.ConvertRealWorldToProjective(2, pt, projectedPt);
				// printf("Projective:x=%f,y=%f, z=%f\n", pt[0].X, pt[0].Y, pt[0].Z);

				float d = dist(projectedPt[0], projectedPt[1]);

				float x = (projectedPt[0].X + projectedPt[1].X)/2;
				float y = (projectedPt[0].Y + projectedPt[1].Y)/2;
				//printf("x=%f,y=%f\n", x, y);

				// draw kamehameha
				float s = d/2-30;
				static time_t kameStart;
				static bool kameStarted = false;
				time_t current;
				if (reald<250){
					if (!kameStarted){
						time(&kameStart);
						kameStarted=true;
						mciSendCommand(g_mop_kame1.wDeviceID,MCI_STOP,0,0);
						mciSendCommand(g_mop_kame1.wDeviceID,MCI_SEEK,MCI_SEEK_TO_START,0);
						mciSendCommand(g_mop_kame1.wDeviceID, MCI_PLAY, 0, 0);
					}else{
						time(&current);
						int diff = (int)difftime(current, kameStart);
						// printf("%d\n", diff);
						g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(aUsers[i], XN_SKEL_LEFT_WRIST, jointLw);
						g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(aUsers[i], XN_SKEL_RIGHT_WRIST, jointRw);
						g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(aUsers[i], XN_SKEL_LEFT_SHOULDER, jointLs);
						g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(aUsers[i], XN_SKEL_RIGHT_SHOULDER, jointRs);
						g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(aUsers[i], XN_SKEL_LEFT_ELBOW, jointLe);
						g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(aUsers[i], XN_SKEL_RIGHT_ELBOW, jointRe);

						float ld = degree(jointLe.position, jointLs.position, pt[0]);
						float rd = degree(jointRe.position, jointRs.position, pt[1]);
						printf("ld=%f, rd=%f\n", ld, rd);
						// ñ{óàÇÕî{ó¶Ççló∂ÇµÇƒãóó£Çë´Ç∑Ç◊Ç´
						s+=diff*10;
						if(abs(ld - PI)<0.5){
							printf("ha!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

							DWORD            dwSecond;
							MCI_STATUS_PARMS mciStatus;

							mciStatus.dwItem = MCI_STATUS_MODE;
							mciSendCommand(g_mop_kame2.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&mciStatus);
							
							if (mciStatus.dwReturn != MCI_MODE_PLAY){
								mciSendCommand(g_mop_kame2.wDeviceID,MCI_STOP,0,0);
								mciSendCommand(g_mop_kame2.wDeviceID,MCI_SEEK,MCI_SEEK_TO_START,0);
								mciSendCommand(g_mop_kame2.wDeviceID, MCI_PLAY, 0, 0);
							}
							mciSendCommand(g_mop_kame1.wDeviceID,MCI_STOP,0,0);

							XnPoint3D pts[2];
							XnPoint3D tpoints[2] = {jointLe.position, jointLs.position};
							g_DepthGenerator.ConvertRealWorldToProjective(2, tpoints, pts);
							ha(s*0.8, 1000, x, y, pts[0].X-pts[1].X,  pts[0].Y-pts[1].Y);
						}
					}
					ki(x,y,s);
				}else{
					kameStarted=false;
				}
			}
		}
	}

	// Swap the OpenGL display buffers
	glutSwapBuffers();
}

void glutIdle (void)
{
	if (g_bQuit) {
		CleanupExit();
	}

	// Display the frame
	glutPostRedisplay();
}

void glutKeyboard (unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:
		CleanupExit();
	case 'i':
		// Print label?
		g_bPrintID = !g_bPrintID;
		break;
	case 'l':
		// Print ID & state as label, or only ID?
		g_bPrintState = !g_bPrintState;
		break;
	}
}
void glInit (int * pargc, char ** argv)
{
	glutInit(pargc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(GL_WIN_SIZE_X, GL_WIN_SIZE_Y);
	glutCreateWindow ("Kamehameha Demo");
	// glutFullScreen();
	glutSetCursor(GLUT_CURSOR_NONE);

	glutKeyboardFunc(glutKeyboard);
	glutDisplayFunc(glutDisplay);
	glutIdleFunc(glutIdle);

	//glEnable(GL_DEPTH_TEST);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL); 

	// lighting
	//glEnable(GL_LIGHTING);
	//glEnable(GL_LIGHT0);
	//glEnable(GL_COLOR_MATERIAL);
}

#define SAMPLE_XML_PATH "SamplesConfig.xml"

#define CHECK_RC(nRetVal, what)										\
	if (nRetVal != XN_STATUS_OK)									\
{																\
	printf("%s failed: %s\n", what, xnGetStatusString(nRetVal));\
	return nRetVal;												\
}

int main(int argc, char **argv)
{
	printf("start\n");

	static MCI_OPEN_PARMS mop;

	mop.lpstrDeviceType=TEXT("WaveAudio");
	mop.lpstrElementName=TEXT("dragon_bgm.wav");
	mciSendCommand(NULL,MCI_OPEN,MCI_OPEN_TYPE | MCI_OPEN_ELEMENT,(DWORD)&mop);
	mciSendCommand(mop.wDeviceID,MCI_PLAY,0,0);

	g_mop_kame1.lpstrDeviceType=TEXT("WaveAudio");
	g_mop_kame1.lpstrElementName=TEXT("kamehameha_1.wav");
	mciSendCommand(NULL,MCI_OPEN,MCI_OPEN_TYPE | MCI_OPEN_ELEMENT,(DWORD)&g_mop_kame1);

	g_mop_kame2.lpstrDeviceType=TEXT("WaveAudio");
	g_mop_kame2.lpstrElementName=TEXT("kamehameha_2.wav");
	mciSendCommand(NULL,MCI_OPEN,MCI_OPEN_TYPE | MCI_OPEN_ELEMENT,(DWORD)&g_mop_kame2);

	XnStatus nRetVal = XN_STATUS_OK;

	if (argc > 1)
	{
		nRetVal = g_Context.Init();
		CHECK_RC(nRetVal, "Init");
		nRetVal = g_Context.OpenFileRecording(argv[1]);
		if (nRetVal != XN_STATUS_OK)
		{
			printf("Can't open recording %s: %s\n", argv[1], xnGetStatusString(nRetVal));
			return 1;
		}
	}
	else
	{
		nRetVal = g_Context.InitFromXmlFile(SAMPLE_XML_PATH);
		CHECK_RC(nRetVal, "InitFromXml");
	}

	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_DEPTH, g_DepthGenerator);
	CHECK_RC(nRetVal, "Find depth generator");
	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_IMAGE, g_ImageGenerator);
	CHECK_RC(nRetVal, "Find image generator");
	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_USER, g_UserGenerator);
	if (nRetVal != XN_STATUS_OK)
	{
		nRetVal = g_UserGenerator.Create(g_Context);
		CHECK_RC(nRetVal, "Find user generator");
	}

	XnCallbackHandle hUserCallbacks, hCalibrationCallbacks, hPoseCallbacks, hHandsCallbacks;
	if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON))
	{
		printf("Supplied user generator doesn't support skeleton\n");
		return 1;
	}
	g_UserGenerator.RegisterUserCallbacks(User_NewUser, User_LostUser, NULL, hUserCallbacks);
	g_UserGenerator.GetSkeletonCap().RegisterCalibrationCallbacks(UserCalibration_CalibrationStart, UserCalibration_CalibrationEnd, NULL, hCalibrationCallbacks);

	if (g_UserGenerator.GetSkeletonCap().NeedPoseForCalibration())
	{
		g_bNeedPose = TRUE;
		if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION))
		{
			printf("Pose required, but not supported\n");
			return 1;
		}
		g_UserGenerator.GetPoseDetectionCap().RegisterToPoseCallbacks(UserPose_PoseDetected, NULL, NULL, hPoseCallbacks);
		g_UserGenerator.GetSkeletonCap().GetCalibrationPose(g_strPose);
	}

	g_UserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);

	nRetVal = g_Context.StartGeneratingAll();
	CHECK_RC(nRetVal, "StartGenerating");

	g_DepthGenerator.GetMetaData(g_depthMD);
	g_ImageGenerator.GetMetaData(g_imageMD);

	// Texture map init
	g_nTexMapX = (((unsigned short)(g_depthMD.FullXRes()-1) / 512) + 1) * 512;
	g_nTexMapY = (((unsigned short)(g_depthMD.FullYRes()-1) / 512) + 1) * 512;
	g_pTexMap = (XnRGB24Pixel*)malloc(g_nTexMapX * g_nTexMapY * sizeof(XnRGB24Pixel));

	glInit(&argc, argv);
	glutMainLoop();

	g_Context.Shutdown();
}
