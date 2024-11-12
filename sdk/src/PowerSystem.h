#define POWER_FREQ			60		// ���������� ����. �Ǽ��� ����Ϸ��� ��ȯ �ʿ�

#define PHASE_A				0
#define PHASE_B				1
#define PHASE_C				2
#define NUM_BASIC_PHASE		3

#define GRID_V_154kV		0
#define GRID_V_345kV		1

// PI
#define PI					3.1415926535

// Radian ���� degree ������ �ٲٱ� ���� ����
// 180/PI �ε�, �̰��� �״�� define���� �ϸ�, �����ϴ� ������ �Ź� �Ǽ� �����Ⱑ �߻�
// ����, �� ���� ����Ͽ� �ٷ� ������ �� �ֵ��� ����
#define RADIAN_TO_DEGREE	57.2957914	// (180./PI)

// Degree ���� radian ������ �ٲٱ� ���� ����
// PI/180 �ε�, �̰��� �״�� define���� �ϸ�, �����ϴ� ������ �Ź� �Ǽ� �����Ⱑ �߻�
// ����, �� ���� ����Ͽ� �ٷ� ������ �� �ֵ��� ����
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

