clear
close all
clc

% need 8 colors
colors = [27,158,119;...
217,95,2;...
117,112,179;...
231,41,138;...
102,166,30;...
230,171,2;...
166,118,29;...
102,102,102] / 255;
set(groot, 'DefaultAxesColorOrder', colors)

%% Input
campaign = "results/scheduler_comparison_v1/data/0a3ec207-581a-4019-9d53-44fc3a7099ad";

pkts = readtable(fullfile(campaign, "packetsTrace.csv"));
pkts.TxTimestamp_s = pkts.TxTimestamp_ns / 1e9; % TxTimestamp_s is actually in [ns]
pkts.RxTimestamp_s = pkts.RxTimestamp_ns / 1e9; % RxTimestamp_s is actually in [ns]
% Add delay info
pkts.Delay_s = pkts.RxTimestamp_s - pkts.TxTimestamp_s;

app = readtable(fullfile(campaign, "appTrace.csv"));
app.Timestamp_s = app.Timestamp_ns / 1e9; % Timestamp_s is actually in [ns]

sps = readtable(fullfile(campaign, "spTrace.csv"));
sps.Timestamp_s = sps.Timestamp_ns / 1e9; % Timestamp_s is actually in [ns]

queue = readtable(fullfile(campaign, "queueTrace.csv"));
queue.Timestamp_s = queue.Timestamp_ns / 1e9; % Timestamp_s is actually in [ns]

%% Global params
nodeId = 1; % reference STA
sta1Mask = pkts.SrcNodeId == nodeId;
numStas = 4;

%% Compute DTI structure
dtiStructure = struct();
srcNodeIds = unique(sps.SrcNodeId);
for i = 1:length(srcNodeIds)
    id = srcNodeIds(i);
    dtiStructure(i).id = id;
    dtiStructure(i).start = sps.Timestamp_s(sps.SrcNodeId == id & sps.isStart == 1);
    dtiStructure(i).end = sps.Timestamp_s(sps.SrcNodeId == id & sps.isStart == 0);
    
    if length(dtiStructure(i).start) > length(dtiStructure(i).end)
        % Proabably simulation end truncated last SP end
        dtiStructure(i).end(end+1) = dtiStructure(i).start(end); % end last SP immediately
    end
    
    if id == 0
        biStart = [0; dtiStructure(i).end];
    end
end

biDuration_s = biStart(2) - biStart(1);

%%
figure
stem(pkts.TxTimestamp_s(sta1Mask), pkts.PktSize_B(sta1Mask), 'DisplayName', sprintf('TxTimestamp SrcNodeId %d', nodeId)); hold on
stem(pkts.RxTimestamp_s(sta1Mask), pkts.PktSize_B(sta1Mask), 'DisplayName', sprintf('RxTimestamp SrcNodeId %d', nodeId))
legend('show', 'Location', 'southeast')
xlabel('Time [s]')
ylabel('PacketSize [B]')
title(strrep(campaign, '_', '\_'))
% xlim(biStart([0,1]+10))

%%
figure
stem(pkts.RxTimestamp_s(sta1Mask), pkts.Delay_s(sta1Mask) * 1e3)
xlabel('RxTime [s]')
ylabel('Delay [ms]')
title(strrep(campaign, '_', '\_'))
legend(sprintf('SrcNodeId %d', nodeId))

%%
figure
histogram(log10(pkts.Delay_s(sta1Mask)), 'DisplayName', 'Delay'); hold on
histogram(log10(diff(pkts.TxTimestamp_s(sta1Mask))), 'DisplayName', 'Inter-Departure Time')
histogram(log10(diff(pkts.RxTimestamp_s(sta1Mask))), 'DisplayName', 'Inter-Arrival Time')
legend('show')
xlabel('Time [s]')
ylabel('Bin Count')
xticks(-5:0)
xticklabels(strcat("$10^{",string(get(gca,'XTick')),"}$"))

%%
height = biDuration_s * 1e3;

figure
plotDti(dtiStructure, height)
set(gca,'ColorOrderIndex', 1)
for i = 1:numStas
    mask = pkts.SrcNodeId == i;
    p(i) = stem(pkts.RxTimestamp_s(mask), pkts.Delay_s(mask) * 1e3, 'DisplayName', sprintf('SrcNodeId %d', i)); hold on
end
legend(p)
xlabel('Time [s]')
ylabel('Delay [ms]')
title(strrep(campaign, '_', '\_'))

%%
thr_mbps = zeros(length(biStart), numStas);
for id = 1:numStas
    staMask = pkts.SrcNodeId == id;
    for tIdx = 2:length(biStart)
        startTime = biStart(tIdx-1);
        endTime = biStart(tIdx);
        mask = staMask & (startTime <= pkts.RxTimestamp_s) & (pkts.RxTimestamp_s < endTime);
        totRxBytes = sum(pkts.PktSize_B(mask));
        thr_mbps(tIdx, id) = totRxBytes * 8 / biDuration_s / 1e6;
    end
end

figure
plot(biStart, thr_mbps)
xlabel('Time [s]')
ylabel('Avg Throughput per BI [Mbps]')
title(strrep(campaign, '_', '\_'))
legend(strcat("SrcNodeId ", string(1:numStas)), 'NumColumns', 2, 'Location', 'southeast')

%%
figure
boxplot(pkts.Delay_s * 1e3, pkts.SrcNodeId)
xlabel('SrcNodeId')
ylabel('Delay [ms]')

%%
groupedPkts = grpstats(pkts, 'SrcNodeId', {'mean', 'std'});

figure
bar(groupedPkts.SrcNodeId, groupedPkts.mean_Delay_s*1e3); hold on
xlabel('SrcNodeId')
ylabel('Mean Delay [ms]')

errorbar(groupedPkts.SrcNodeId, groupedPkts.mean_Delay_s*1e3, groupedPkts.std_Delay_s*1e3, 'k', 'LineStyle', 'none')

%%
height = biDuration_s * 1e3;

figure
plotDti(dtiStructure, height)

set(gca,'ColorOrderIndex', 1)
for i = 1:numStas
    mask = app.SrcNodeId == i;
    p(i) = stem(app.Timestamp_s(mask), app.PktSize(mask)/10, 'DisplayName', sprintf('SrcNodeId %d', i)); hold on
end
legend(p)
xlabel('Time [s]')
ylabel('Delay [ms]')
title(strrep(campaign, '_', '\_'))

%% PLOT QUEUE
height = biDuration_s * 1e3;

figure
plotDti(dtiStructure, height)

set(gca,'ColorOrderIndex', 1)
for i = 1:numStas
    mask = app.SrcNodeId == i;
    stairs(queue.Timestamp_s(mask), queue.queueSize_pkts(mask)/10); hold on
end
legend(strcat("SrcNodeId ", string(1:numStas)), 'NumColumns', 2, 'Location', 'southeast')
xlabel('Time [s]')
ylabel('Queue Size [pkts]')
title(strrep(campaign, '_', '\_'))

%% UTILS
function plotSp(spStruct, height, alpha)
assert(length(spStruct.start) == length(spStruct.end))
nSps = length(spStruct.start);

x = zeros(1, nSps*4);
x(1:4:end) = spStruct.start;
x(2:4:end) = spStruct.start;
x(3:4:end) = spStruct.end;
x(4:4:end) = spStruct.end;

y = repmat([0,1,1,0] * height, 1, nSps);
area(x, y, 'FaceAlpha', alpha, 'DisplayName', sprintf('SrcNodeId %d', spStruct.id))
end


function plotDti(dtiStructure, height)
alpha = 0.5;

for i = 1:length(dtiStructure)
    if dtiStructure(i).id > 0 && dtiStructure(i).id < 255
        set(gca,'ColorOrderIndex', dtiStructure(i).id)
        plotSp(dtiStructure(i), height, alpha); hold on
    end
end

end