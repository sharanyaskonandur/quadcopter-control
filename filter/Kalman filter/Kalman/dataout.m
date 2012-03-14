clear all;
load out.dat;
phi=out(:,1);
phi_kalman=out(:,2);
p_kalman=out(:,3);
p_bias=out(:,4);
phi_kalman_int=out(:,5);
p_kalman_int=out(:,6);
p_bias_int=out(:,7);
figure;
plot(phi);
hold all;
plot(phi_kalman);
figure;
plot(phi_kalman);
hold all;
plot(phi_kalman_int);
figure;
plot(p_bias);
hold all;
plot(p_bias_int);