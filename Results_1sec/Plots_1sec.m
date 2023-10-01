% This script created to plot the simulation data for ms timer

% Author: Tzouvaras Evangelos
% Email: tzouevan@ece.auth.gr

%% Clear variables and command window
clear;
clc;

%% Load the data from .txt file
periods = load('Periods_measurement_1sec.txt')/1000;
execution_times = load('Execution_times_1sec.txt')/1000;
drift_errors = load('Drift_error_1sec.txt')/1000;

%% Plot data period
for i=1:length(periods)
    num_iterations(i) = i;
end

figure;
stem(num_iterations, periods);
xlabel('# of Iteration'); 
ylabel('Period (ms)');
ylim([0 5000]);
title('Timer actual periods for 1sec timer');

%% Plot execution times
for i=1:length(execution_times)
    num_iterations(i) = i;
end
figure;
stem(num_iterations, execution_times);
xlabel('# of Iteration'); 
ylabel('Execution Time (ms)');
ylim([0 500]);
title('Execution time for 1sec timer');

%% Plot drift error
for i=1:length(drift_errors)
    num_iterations(i) = i;
end
figure;
stem(num_iterations, drift_errors);
xlabel('# of Iteration'); 
ylabel('Drift errors time (ms)');
ylim([0 1000]);
title('Drift error for 1sec timer');

%% Calculate the mean values
mean_period = mean(periods);
mean_execution_time = mean(execution_times);
mean_drift_error = mean(drift_errors);
fprintf('\n\n=============================================================');
fprintf('\nMean Period(ms):%d', mean_period);
fprintf('\nMean Execution Time(ms):%d', mean_execution_time);
fprintf('\nMean Drift Error(ms):%d', mean_drift_error);
fprintf('\n=============================================================');

