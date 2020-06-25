clear
close all
clc

%%
folder = 'results/scheduler_comparison_v1';

% Read json
fid = fopen(fullfile(folder, 'scheduler_comparison_v1.json')); 
raw = fread(fid,inf); 
str = char(raw'); 
fclose(fid);
sims = jsondecode(str);

simsFields = fields(sims.results);

%% Pre-process
data = struct();
for i = 1:length(simsFields)
    field = simsFields{i};
    sim = sims.results.(field);
    
    id = sim.meta.id;
    tab = readtable(fullfile(folder, 'data', id, 'results.csv'));
    
    data(i).id = id;
    data(i).results = tab;
    data(i).schedulerTypeIdx = sim.params.schedulerTypeIdx;
    data(i).mpduAggregation = sim.params.mpduAggregation;
end

% sort
data = struct2table(data);
data = sortrows(data, ["mpduAggregation", "schedulerTypeIdx"]);
data = table2struct(data);

%% NO-AMPDU
noAmpduIdx = find([data.mpduAggregation] == 0);
legendLabels = getLegendLabels(data(noAmpduIdx));

% Plots
metric = "AvgDelay";
metricData = getData(data(noAmpduIdx), metric);

figure
bar(metricData * 1e3)
xlabel('STA ID')
ylabel('Avg Delay [ms]')
legend(legendLabels, 'Location', 'best')


metric = "AvgJitter";
metricData = getData(data(noAmpduIdx), metric);

figure
bar(metricData * 1e3)
xlabel('STA ID')
ylabel('Avg Jitter [ms]')
legend(legendLabels, 'Location', 'best')


metric = "AvgThroughput";
metricData = getData(data(noAmpduIdx), metric);

figure
bar(metricData)
xlabel('STA ID')
ylabel('Avg Throughput [Mbps]')
legend(legendLabels, 'Location', 'best')

%% half-max AMPDU
ampduIdx = find([data.mpduAggregation] == 131072);
legendLabels = getLegendLabels(data(ampduIdx));

% Plots
metric = "AvgDelay";
metricData = getData(data(ampduIdx), metric);

figure
bar(metricData * 1e3)
xlabel('STA ID')
ylabel('Avg Delay [ms]')
legend(legendLabels, 'Location', 'best')


metric = "AvgJitter";
metricData = getData(data(ampduIdx), metric);

figure
bar(metricData * 1e3)
xlabel('STA ID')
ylabel('Avg Jitter [ms]')
legend(legendLabels, 'Location', 'best')


metric = "AvgThroughput";
metricData = getData(data(ampduIdx), metric);

figure
bar(metricData)
xlabel('STA ID')
ylabel('Avg Throughput [Mbps]')
legend(legendLabels, 'Location', 'best')

%% MAX AMPDU
ampduIdx = find([data.mpduAggregation] == 262143);
legendLabels = getLegendLabels(data(ampduIdx));

% Plots
metric = "AvgDelay";
metricData = getData(data(ampduIdx), metric);

figure
bar(metricData * 1e3)
xlabel('STA ID')
ylabel('Avg Delay [ms]')
legend(legendLabels, 'Location', 'best')


metric = "AvgJitter";
metricData = getData(data(ampduIdx), metric);

figure
bar(metricData * 1e3)
xlabel('STA ID')
ylabel('Avg Jitter [ms]')
legend(legendLabels, 'Location', 'best')


metric = "AvgThroughput";
metricData = getData(data(ampduIdx), metric);

figure
bar(metricData)
xlabel('STA ID')
ylabel('Avg Throughput [Mbps]')
legend(legendLabels, 'Location', 'best')

%% UTILS
function data = getData(results, metric)

nScenarios = length(results);
nStas = height(results(1).results);
data = nan(nStas, nScenarios);
for i = 1:nScenarios
    scenarioData = results(i).results.(metric);
    data(1:length(scenarioData), i) = scenarioData;
end

end


function legendLabels = getLegendLabels(data)

legendLabels = [""];
for i = 1:length(data)
    switch(data(i).schedulerTypeIdx)
        case 0
            label = "CBAP Only";
        case 1
            label = "Basic";
        otherwise
            label = sprintf("Period: BI/%d", data(i).schedulerTypeIdx);
    end
            
    legendLabels(i) = label;
end

end