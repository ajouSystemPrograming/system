#define POUT 23 // 초음파센서
#define PIN 24 // 초음파센서
//#define POUT2 16 // LED/
#define PWM 0
//#define POUT 21 // 버튼
//#define PIN 20 // 버튼


// 거리에 따라 led를 점등시키는 함수
int led_breathing() {

	// Enable GPIO pins
	if (-1 == GPIOExport(POUT) || -1 == GPIOExport(PIN)) {
		printf("gpio export err\n");
		return (1);
	}
	// wait for writing to export file
	usleep(100000);

	// Set GPIO directions
	if (-1 == GPIODirection(POUT, OUT) || -1 == GPIODirection(PIN, IN)) {
		printf("gpio direction err\n");
		return (2);
	}
	
	if (-1 == PWMExport(PWM)) {
		printf("PWM export err\n");
		return (4);
	}
	// wait for writing to export file
	usleep(100000);
	if (-1 == PWMWritePeriod(PWM, 10000000)) {
		printf("PWM period err\n");
		return (5);
	}
	usleep(10000);

	if (-1 == PWMWriteDutyCycle(PWM, 0)) {
		printf("PWM duty cycle err\n");
		return (6);
	}
	if (-1 == PWMEnable(PWM)) {
		printf("PWM duty cycle err\n");
		return (7);
	}
	
	// init ultrawave trigger
	GPIOWrite(POUT, 0);
	usleep(10000);
	// start
	clock_t start_t, end_t;
	double time;
	double distance = 0;

	while(1) {
		int isPic = 0; // 카메라 파이에서 사진을 찍었는지 여부를 확인하기 위한 숫자.
		if (-1 == read(sock, &isPic, sizeof(isPic)))
			error_handling("pic read() error");
		if (read) {
			printf("Picture is captured.\nStop sensoring distance.\n")
			return 0;
		}


		if (-1 == GPIOWrite(POUT, 1)) {
			printf("gpio write/trigger err\n");
			return (3);
		}

		// 1sec == 1000000ultra_sec, 1ms = 1000ultra_sec
		usleep(10);
		GPIOWrite(POUT, 0);

		while (GPIORead(PIN) == 0) {
			start_t = clock();
		}
		while (GPIORead(PIN) == 1) {
			end_t = clock();
		}

		time = (double)(end_t - start_t) / CLOCKS_PER_SEC;  // ms
		printf("time : %.4lf\n", time);
		distance = time / 2 * 34000;
		
		printf("distance : %.2lfcm\n", distance);
		if (-1 == write(sock, &distance, sizeof(distance)))
			error_handling("distance write() error");
		
		// '가장 최적의 거리'일 때를 20~30cm이라 하자.
		if (distance < 0) { // ~ 0
			printf("@@Sensor err! Distance should be positive\n");
			PWMWriteDutyCycle(PWM, 00000);
			
		} else if (distance >= 40.00) { // 40 ~ 50
			printf("@@Please put it clooooser\n");
			PWMWriteDutyCycle(PWM, 00000);

		} else if (distance < 20) { // 0 ~ 20
			printf("@@Please put it farther\n");
			PWMWriteDutyCycle(PWM, distance * 150000);
			
		} else if (distance < 30){ // 20 ~ 30
			printf("@@Proper distance!!\n");
			PWMWriteDutyCycle(PWM, 10000000);
			
		} else if (distance >= 30.00) { // 30 ~ 50
			printf("@@Please put it closer\n");
			PWMWriteDutyCycle(PWM, (50 - distance) * 150000);
		} 
		
	
		

		usleep(1000000);
	}

	// Disable GPIO pins
	if (-1 == GPIOUnexport(POUT) || -1 == GPIOUnexport(PIN)) return (4);

	printf("complete\n");
}

int main(int argc, char *argv[]) {
	mask[0]=1;
	for(int i = 1; i < 24; i++)
		mask[i] = mask[i-1]*6;
	
	// 소켓 연결을 위한 코드들
	
	struct sockaddr_in serv_addr;
	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1) 
		error_handling("socket() error");
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");
	printf("Connection established\n");
	
	// 초음파센서 값 전달
	if (0 == led_breathing()) {
		printf("led_breathing completed.\n");
	}
	
	// 이제 쓰레드 실행을 위한 코드들
	pthread_t p_thread[3];
	int thr_id;
	int thr_id2;
	int thr_id3;
	int status;
	char p1[] = "receiving_thread";
	char p2[] = "sending_thread";
	char pM[] = "calculating_thread";

	// 쓰레드 예외처리
	//thr_id = pthread_create(&p_thread[0], NULL, receiving_thread, (void *)p1);
	thr_id = pthread_create(&p_thread[0], NULL, receiving_thread, NULL);
	if (thr_id < 0) {
		perror("reveiving_thread created error : ");
		exit(0);
	}
	thr_id2 = pthread_create(&p_thread[1], NULL, sending_thread, NULL);
	if (thr_id2 < 0) {
		perror("sending_thread created error : ");
		exit(0);
	}
	/*
	 * thr_id3 = pthread_create(&p_thread[2], NULL, cal, NULL);
	if (thr_id3 < 0) {
		perror("reveiving_thread created error : ");
		exit(0);
	}
	*/
	//cal();
	pthread_join(p_thread[0], (void **)&status);
	pthread_join(p_thread[1], (void **)&status);
	//pthread_join(p_thread[2], (void **)&status);

	close(sock);


	return (0);
}
