import './App.css';
import React, { useState, useEffect } from 'react';
import ReactJson from 'react-json-view' //https://github.com/microlinkhq/react-json-view
import { Tooltip } from 'react-tooltip' //https://github.com/ReactTooltip/react-tooltip#readme

import API from './api/api.ts'
import validateURL from './helper_funcs/validate_url.ts';

function App() {
  const [inProcessData, setInProcessData] = useState([]);
  const [processedData, setProcessedData] = useState([]);
  const [estimateData, setEstimateData] = useState({est_sec: 0,num_tokens: 0, num_comments: 0});
  const [videoUrl, setVideoUrl] = useState('');
  const [optionDescription, setOptionDescription] = useState('');
  const [isEstimateDisabled, setIsEstimateDisabled] = useState(true);
  const [isProcessDisabled, setIsProcessDisabled] = useState(true);

  const loadDocuments = async () => {
    const resp = await API.getVideoDocuments();
    try {
      setInProcessData(resp.in_progress);
      setProcessedData(resp.completed);
    } catch {

    }
  }

  useEffect(() => {
    // Initial fetch when the component mounts
    loadDocuments();
    // Set up an interval to fetch data every 15 seconds
    const intervalDelay = 15000; // 15 seconds in milliseconds
    const intervalId = setInterval(()=>{loadDocuments()}, intervalDelay);
    // Clean up the interval when the component unmounts
    return () => clearInterval(intervalId);
  }, []);
  
  useEffect(() => {
    setIsProcessDisabled(true);
    setIsEstimateDisabled(true);
    let validate = validateURL(videoUrl);
    if(validate.isValid) {
      setIsEstimateDisabled(false);
    }
  }, [videoUrl, isEstimateDisabled]);

  const handleEstimate = async () => {
    try {
      // setIsEstimateDisabled(true);
      const resp = await API.estimateVideo(videoUrl,optionDescription);
      setEstimateData(resp)
      // setIsEstimateDisabled(true);
    } catch (error) {
      console.error('Error fetching data:', error);
    } finally {
      setIsProcessDisabled(false); // Re-enable estimate button after estimate
    }
  };

  const handleProcess = async () => {
    try {
      const resp = await API.processVideo(videoUrl, optionDescription);
      loadDocuments();
    } catch (error) {
      console.error('Error fetching data:', error);
    } finally {
      // setIsProcessDisabled(false); // Re-enable estimate button after estimate
    }
  };

  const inputForm = () => {
    return (
        <form className='form-container'>
            <div className="input-container">
                <h3 className="input-title">Enter a Youtube URL</h3>
                <input
                    type="text"
                    value={videoUrl}
                    onChange={(e) => setVideoUrl(e.target.value)}
                    required
                />
            </div>
            <div className="input-container">
                <h3 className="input-title">Option Description</h3>
                <Tooltip id="optionalDescriptionTooltip" />
                <textarea
                    data-tooltip-id="optionalDescriptionTooltip"
                    data-tooltip-content="Orient the description toward the type of content suggestions you're looking for to get better results"
                    data-tooltip-place='right'
                    placeholder="If left blank, the description on the video itself will be used to prompt the LLM."
                    rows="4"
                    value={optionDescription}
                    onChange={(e) => setOptionDescription(e.target.value)}
                />
            </div>
            <div className="button-container">
                <button
                    type="button"
                    onClick={handleEstimate}
                    disabled={isEstimateDisabled}
                >
                    Estimate
                </button>
                <button
                    type="button"
                    onClick={handleProcess}
                    disabled={isProcessDisabled}
                >
                    Process
                </button>
            </div>
        </form>
    )
  }

  const estimateDisplay = () => {
    return (
      <div className='estimate-container'>
        <ReactJson 
            src={estimateData}
            name={"estiamte"}
            collapsed={false}
            collapseStringsAfterLength={80}
            enableClipboard={false}
            displayDataTypes={false}
            theme="monokai"
        />
      </div>
    )
  }

  const dataDisplay = (data) => {
    if(!data) return;
    return (
        <div className="scrollable-container">
            {data.map((item, index) => (
                <ReactJson 
                    src={item} 
                    key={index} 
                    name={item.title}
                    collapsed={true}
                    collapseStringsAfterLength={80}
                    enableClipboard={false}
                    displayDataTypes={false}
                    theme="monokai"
                />
            ))}
        </div>
    )
  }

  return (
    <div className="app">
      {inputForm()}
      {estimateDisplay()}
      <div className="data-container">
        <div className="process-container">
            <h3>In Progress</h3>
            {dataDisplay(inProcessData)}
        </div>
        <div className="process-container">
            <h3>Compeleted</h3>
            {dataDisplay(processedData)}
        </div>
      </div>
    </div>
  );
}

export default App;
