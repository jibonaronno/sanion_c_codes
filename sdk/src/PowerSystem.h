#define POWER_FREQ			60		// 정수형으로 선언. 실수로 사용하려면 변환 필요

#define PHASE_A				0
#define PHASE_B				1
#define PHASE_C				2
#define NUM_BASIC_PHASE		3

#define GRID_V_154kV		0
#define GRID_V_345kV		1

// PI
#define PI					3.1415926535

// Radian 값을 degree 값으로 바꾸기 위한 변수
// 180/PI 인데, 이것을 그대로 define으로 하면, 적용하는 곳에서 매번 실수 나누기가 발생
// 따라서, 그 값을 계산하여 바로 적용할 수 있도록 정의
#define RADIAN_TO_DEGREE	57.2957914	// (180./PI)

// Degree 값을 radian 값으로 바꾸기 위한 변수
// PI/180 인데, 이것을 그대로 define으로 하면, 적용하는 곳에서 매번 실수 나누기가 발생
// 따라서, 그 값을 계산하여 바로 적용할 수 있도록 정의
#define DEGREE_TO_RADIAN	0.01745329	// (PI/180.)

#define SQUAREROOT_2		1.41421356
#define SQUAREROOT_3		1.73205081


#define MAX_PD_CHANNELS		24

#define MAX_GIS_ACQS		9

#define MAX_MTR_OLTCS		3
#define MAX_MTR_BUSHINGS	2
#define MAX_MTR_TANKS		6


typedef struct {
	float m_fReal;
	float m_fImag;
	float m_fMag;
	float m_fAng;
} dsPhasor;


typedef struct {
	U8	m_u8GridVoltage;
} dsPowerSystem;

