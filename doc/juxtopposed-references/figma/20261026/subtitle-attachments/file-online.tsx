const assetPathPrefix = "https://www.figma.com/api/mcp/asset/c046251b-5171-44fd-bd79-c5b8ade320ce";
const imgFrame1 = `${assetPathPrefix}/1b0f7.svg`;
const imgFrame = `${assetPathPrefix}/764c9.svg`;
const imgFrame2 = `${assetPathPrefix}/d74b3.svg`;
const imgFrame3 = `${assetPathPrefix}/a8d92.svg`;
const imgFrame4 = `${assetPathPrefix}/9c2da.svg`;
const imgFrame5 = `${assetPathPrefix}/8abe2.svg`;

type IconProps = {
  className?: string;
  direction?: "default" | "down";
  eq?: "default";
  name?: "settings" | "chevron" | "check" | "search";
  state?: "default" | "active";
  twoTone?: "no";
  value?: "default";
};

function Icon({ className, direction = "default", eq = "default", name = "settings", state = "default", twoTone = "no", value = "default" }: IconProps) {
  const isCheckAndDefaultAndActiveAndDefaultAndNo = name === "check" && direction === "default" && state === "active" && eq === "default" && twoTone === "no";
  const isCheckAndDefaultAndDefaultAndDefaultAndNo = name === "check" && direction === "default" && state === "default" && eq === "default" && twoTone === "no";
  const isChevronAndDownAndDefaultAndDefaultAndNo = name === "chevron" && direction === "down" && state === "default" && eq === "default" && twoTone === "no";
  const isSearchAndDefaultAndDefaultAndDefaultAndNo = name === "search" && direction === "default" && state === "default" && eq === "default" && twoTone === "no";
  return (
    <div className={className || "content-stretch flex items-center justify-center relative"} id={isSearchAndDefaultAndDefaultAndDefaultAndNo ? "node-856_2147" : isCheckAndDefaultAndDefaultAndDefaultAndNo ? "node-494_2400" : isCheckAndDefaultAndActiveAndDefaultAndNo ? "node-494_2394" : isChevronAndDownAndDefaultAndDefaultAndNo ? "node-71_868" : "node-42_355"}>
      <div className={`relative shrink-0 ${isChevronAndDownAndDefaultAndDefaultAndNo ? "flex items-center justify-center" : "size-[24px]"}`} id={isSearchAndDefaultAndDefaultAndDefaultAndNo ? "node-856_2148" : isCheckAndDefaultAndDefaultAndDefaultAndNo ? "node-494_2401" : isCheckAndDefaultAndActiveAndDefaultAndNo ? "node-494_2395" : isChevronAndDownAndDefaultAndDefaultAndNo ? "node-71_869" : "node-44_413"}>
        {direction === "default" && eq === "default" && twoTone === "no" && ((name === "settings" && state === "default") || (name === "check" && state === "active") || (name === "search" && state === "default")) && <img alt="" className="absolute block inset-0 max-w-none size-full" src={name === "search" && state === "default" && eq === "default" && twoTone === "no" ? imgFrame3 : name === "check" && state === "active" && eq === "default" && twoTone === "no" ? imgFrame2 : imgFrame1} />}
        {isChevronAndDownAndDefaultAndDefaultAndNo && (
          <div className="-scale-y-100 flex-none rotate-180">
            <div className="relative size-[24px]" data-name="Frame">
              <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame} />
            </div>
          </div>
        )}
      </div>
    </div>
  );
}

type OriginProps = {
  className?: string;
  state?: "from opensub";
};

function Origin({ className, state = "from opensub" }: OriginProps) {
  return (
    <button className={className || "bg-[#151515] content-stretch cursor-pointer flex items-center justify-between pl-[12px] pr-[8px] py-[5px] relative rounded-[6px] w-[200px]"} data-node-id="1963:5469">
      <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] text-left whitespace-nowrap" data-node-id="1963:5470">
        From OpenSubtitles
      </p>
      <Icon className="content-stretch flex items-center justify-center relative shrink-0" direction="down" name="chevron" />
    </button>
  );
}

type TargetProps = {
  className?: string;
  state?: "to this file";
};

function Target({ className, state = "to this file" }: TargetProps) {
  return (
    <button className={className || "bg-[#151515] content-stretch cursor-pointer flex items-center justify-between pl-[12px] pr-[8px] py-[5px] relative rounded-[6px] w-[200px]"} data-node-id="1959:5350">
      <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] text-left whitespace-nowrap" data-node-id="1959:5351">
        To This File
      </p>
      <Icon className="content-stretch flex items-center justify-center relative shrink-0" direction="down" name="chevron" />
    </button>
  );
}

export default function AddSubtitlesToPlaylistFolder() {
  return (
    <div className="backdrop-blur-[25px] bg-[rgba(14,14,14,0.95)] content-stretch flex flex-col isolate items-start relative rounded-[14px] shadow-[0px_8px_15px_0px_rgba(0,0,0,0.46)] size-full" data-node-id="1919:9278" data-name="Add Subtitles to Playlist/Folder">
      <div className="bg-[#090909] content-stretch flex flex-col gap-[12px] items-start p-[9px] relative rounded-tl-[14px] rounded-tr-[14px] shrink-0 w-full z-[3]" data-node-id="1919:9279">
        <div className="content-stretch flex items-center justify-between pl-[10px] relative shrink-0 w-full" data-node-id="1919:9280">
          <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#8f8f8f] text-[14px] whitespace-nowrap" data-node-id="1919:9281">
            Add Subtitles
          </p>
          <button className="content-stretch cursor-pointer flex items-center justify-center p-[4px] relative rounded-[4px] shrink-0 w-[32px]" data-node-id="1919:9282" data-name="Icon Wrapper">
            <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I1919:9282;177:88" data-name="Icon">
              <div className="flex items-center justify-center relative shrink-0" data-node-id="I1919:9282;177:88;45:41">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame4} />
                  </div>
                </div>
              </div>
            </div>
          </button>
        </div>
        <div className="content-stretch flex gap-[10px] items-center relative shrink-0 w-full" data-node-id="1919:9283">
          <Target className="bg-[#151515] content-stretch cursor-pointer flex items-center justify-between pl-[12px] pr-[8px] py-[5px] relative rounded-[6px] shrink-0 w-[200px]" />
          <Origin className="bg-[#151515] content-stretch cursor-pointer flex items-center justify-between pl-[12px] pr-[8px] py-[5px] relative rounded-[6px] shrink-0 w-[200px]" />
          <div className="bg-[#151515] content-stretch flex flex-[1_0_0] gap-[10px] h-[34px] items-center min-w-px px-[8px] py-[5px] relative rounded-[6px]" data-node-id="1919:9287">
            <Icon className="content-stretch flex items-center justify-center relative shrink-0" name="search" />
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1919:9289">
              The Wand S01E01
            </p>
          </div>
          <div className="bg-[#151515] content-stretch flex items-center justify-between pl-[12px] pr-[8px] py-[5px] relative rounded-[6px] shrink-0 w-[200px]" data-node-id="1919:9380">
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1919:9381">
              English
            </p>
            <Icon className="content-stretch flex items-center justify-center relative shrink-0" direction="down" name="chevron" />
          </div>
        </div>
      </div>
      <div className="content-stretch flex flex-[1_0_0] items-center min-h-px relative w-full z-[2]" data-node-id="1919:9290">
        <div className="content-stretch cursor-pointer flex flex-[1_0_0] flex-col gap-[4px] h-full items-start min-w-px overflow-clip px-[20px] py-[22px] relative" data-node-id="1919:9291">
          <button className="bg-[#090909] content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="3336:11895" data-name="subtitle one">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I3336:11895;1919:9127">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:11895;1919:9128" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I3336:11895;1919:9128;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] text-left whitespace-nowrap" data-node-id="I3336:11895;1919:9129">
                The_Wand_S01E01_English.srt
              </p>
            </div>
            <Icon className="content-stretch flex items-center justify-center relative shrink-0" name="check" state="active" />
          </button>
          <button className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="3336:11907" data-name="subtitle one">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I3336:11907;1919:9123">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:11907;1919:9124" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I3336:11907;1919:9124;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] text-left whitespace-nowrap" data-node-id="I3336:11907;1919:9125">
                The_Wand_S01E01_EN.srt
              </p>
            </div>
          </button>
        </div>
      </div>
      <div className="content-stretch flex items-center justify-between p-[20px] relative shrink-0 w-full z-[1]" data-node-id="1994:5585">
        <div className="bg-[#1b1b1b] content-stretch flex h-[32px] items-center justify-center px-[8px] py-[4px] relative rounded-[6px] shrink-0 w-[100px]" data-node-id="1994:5586">
          <p className="[word-break:break-word] font-['Inter:Semi_Bold'] font-semibold leading-[normal] not-italic relative shrink-0 text-[#8f8f8f] text-[14px] whitespace-nowrap" data-node-id="1994:5587">
            Cancel
          </p>
        </div>
        <button className="bg-[#371f06] content-stretch cursor-pointer flex h-[32px] items-center justify-center px-[8px] py-[4px] relative rounded-[6px] shrink-0 w-[100px]" data-node-id="1994:5588">
          <p className="[word-break:break-word] font-['Inter:Semi_Bold'] font-semibold leading-[normal] not-italic relative shrink-0 text-[#d37918] text-[14px] text-left whitespace-nowrap" data-node-id="1994:5589">
            Save
          </p>
        </button>
      </div>
    </div>
  );
}