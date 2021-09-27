Fs = 44100; %Sampling Frequency
freq_low = 200; %lowest Frequency for filter
freq_high = 8000; %highest frequency for filter
lowest_stop_freq = freq_low/2;
highest_stop_freq = freq_high*1.25;
num_Filter=20;  %amount of filters used
min_filterOrder=1; %minimal filter order used
max_filterOrder=7; %maximal filter order dont use more than 7 - the vocoder works only for up to 8 coefficients
stopbandAtt=30; %stopband in dB
rippleInPassband = 1; % indB
noise = rand(1,Fs);% 1 sec of white noise for checking filters
file = audioread('Y:\students\Robbiani_Jakob\BA-PROJECT\VOCODER V2\Vocoder\Vocoder\Speech.wav');
audiofile=transpose(file(:,1));

close all %closes already opened figures


filenameweights = 'filterweights.txt';
fileIDweights = fopen(filenameweights,'w');
fprintf(fileIDweights,'Filterweights\r\n');


%for each filterconfiguration
for i=1:num_Filter
    %calculate edges
    edgefrequencies = logspace(log10(freq_low),log10(freq_high),i+1);
    %zero centerfreqs
    centerfrequencies = zeros(1,i);
    centerfrequencies_log = zeros(1,i);
    %for each filter
    for j=1:i
    	%calculate centers
        centerfrequencies(j) = (edgefrequencies(j)+edgefrequencies(j+1))/2;        %(edgefrequencies(j)+edgefrequencies(j+1))/2;
        centerfrequencies_log(j) = geomean([edgefrequencies(j) edgefrequencies(j+1)]);        
    end
    %all centers calculated
    
    %all weights are just set to 1, change if needed
    centerfrequenciesweight = ones(1,i);
    fprintf(fileIDweights,['FilterBank',num2str(i),':\r\n']);
    fprintf(fileIDweights,'\t%1.6g\r\n',centerfrequenciesweight);
    
    
    %create filename
    filename=['FilterBank_',num2str(i),'.txt'];
    %open file for writing
    fileID = fopen(filename,'w');
    %write centerfrequencies
    fprintf(fileID,['Filter in this bank: ',num2str(i),'\r\n']);
    fprintf(fileID,'CENTERFREQUENCIES:\r\n');
    fprintf(fileID,'%6.5g\r\n',centerfrequencies_log);
    %fprintf(fileID, '
    %fclose(fileID);
    filterednoise = zeros(1,Fs);
    
    
    %create figure for display
    fig = figure('Name',['Filterbank for ',num2str(i),' filters']);
    for j=1:i
        
        stopband=[edgefrequencies(j),edgefrequencies(j+1)];
        %create filter
        %[b,a] = butter(2,[edgefrequencies(j),edgefrequencies(j+1)]/Fs*2);
        currentfilterorder = floor(max_filterOrder/2);
        [b,a]=cheby2(currentfilterorder, stopbandAtt, stopband/Fs*2);
        %if you use cheby they probably all will be 6th order
        isStable = isstable(b,a);
        %assert(isStable==1);
        
        %if its not stable we are too high and decrease
        while isStable~=1 && currentfilterorder>=ceil(min_filterOrder/2)
            currentfilterorder = currentfilterorder-1;
            clear b; clear a;
            [b,a]=cheby2(currentfilterorder, stopbandAtt, stopband/Fs*2);
            isStable= isstable(b,a);
        end
        
        [h,f] = freqz(b,a,Fs,Fs);
        h=20*log10(abs(h));
        needslowerorder = 0;
        for k=1:length(h)
            if h(k)>rippleInPassband
                needslowerorder=1;
                %disp([num2str(h(k)),'->filter: ',num2str(i),'_',num2str(j)]);
            end
        end
        %check if we somewhere gain more db than allowed and decrease those filters by one
        if needslowerorder==1 && currentfilterorder>min_filterorder
            currentfilterorder=currentfilterorder-1;
            clear b;clear a;clear h; clear f;
            [b,a]=cheby2(currentfilterorder, stopbandAtt, stopband/Fs*2);
            %check again
            [h,f] = freqz(b,a,Fs,Fs);
            h=20*log10(abs(h));
            needslowerorder = 0;
        elseif needslowerorder==1 && ~(currentfilterorder>min_filterorder)
            error('no stable filter with no gain in passband without going below min_filterorder');
        end
        %check again just to make sure
        for k=1:length(h)
            assert( h(k)<=rippleInPassband);
        end
        %we got a stable filter, display
        %}
        %impz(b,a);
        freqz(b,a,Fs,Fs); hold on
        %periodogram(noise); hold on
        %filterednoise = filterednoise + filter(b,a,noise);
        %periodogram(filterednoise,[],Fs,'power');
        
        %periodogram(filterednoise);
        %data = zeros(1,256);
        %for l=1:256
        %    data(l)=0.5;
        %end
        %disp(data);
        %filteredfile = filter(b,a, data);
        %disp(filteredfile);
        
        
        %if filter is stable write to file
        fprintf(fileID,'/-------------------------------------------------/\r\n');%just for more readability
        fprintf(fileID,['Filterband:',num2str(j),'\r\n']);
        fprintf(fileID,['Numerator Length: ',num2str(length(b)),'\r\n']);
        fprintf(fileID,'\t%.15f\r\n',b); %dont use g otherwise we get exponential notation
        %in this notation we get annoying 0..000 for 0, shouldnt be a
        %problem
        fprintf(fileID,['Denominator Length: ',num2str(length(a)),'\r\n']);
        fprintf(fileID,'\t%.15f\r\n',a);
        fprintf(fileID,'/-------------------------------------------------/\r\n');
        clear a; clear b; clear n; clear Wn;
        
        %fileID = fopen('test.txt','w');
        %fprintf(fileID,'%6.8f\r\n', b);
    end
    ax = findall(gcf, 'Type', 'axes');
    set(ax, 'XScale', 'log');
    set(ax, 'XLim', [100 10000]);
    set(ax, 'YLim', [-100 10]);
    fclose(fileID);
end
%create Lowpass at 100 Hz
filenamelp = 'lowpass_for_envelope.txt';
FileIDlp = fopen(filenamelp,'w');

cutLp = 200;
f=figure('Name',['Lowpass ',num2str(cutLp),'Hz']);
stopAttLp = 40;
clear b; clear a;
%dont use more than 7th order for this
[b,a] = cheby2(3,stopAttLp,cutLp/Fs*2);
%[b,a] = butter(5,cutLp/Fs*2);
%filter(b,a,0.5)




assert(isstable(b,a));
%write to file
freqz(b,a);
fprintf(FileIDlp,['Numerator Length: ',num2str(length(b)),'\r\n']);
fprintf(FileIDlp,'\t%.15f\r\n',b);
fprintf(FileIDlp,['Denominator Length: ',num2str(length(a)),'\r\n']);
fprintf(FileIDlp,'\t%.15f\r\n',a);
fclose(FileIDlp);